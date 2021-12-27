#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}


//由于需要利用作为中介的上传缓冲区来初始化默认缓冲区（D3D12_HEAP_TYPE_DEFAULT）中的数据，
//因此构建下列工具函数，以避免在每次使用默认缓冲区重复地工作。
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // 创建实际的默认缓冲区资源
    auto g = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto h = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &g,
        D3D12_HEAP_FLAG_NONE,
        &h,
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // 为了将CPU端内存中的数据复制到默认缓冲区，
    //我们还需要创建一个处于中介位置的上传堆
    auto i = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto j = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &i,
		D3D12_HEAP_FLAG_NONE,
        &j,
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // 描述我们希望复制到默认缓冲区中的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;//指向某个系统内存块的指针,其中有初始化缓冲区所用的数据。
    subResourceData.RowPitch = byteSize;//对于缓冲区而言,此参数亦为欲复制数据的字节数。
    subResourceData.SlicePitch = subResourceData.RowPitch;//对于缓冲区而言，此参数亦为欲复制数据的字节数。

    //将数据复制到默认缓冲区资源的流程
    // UpdateSubresources辅助函数会先将数据从cpu端的内存中复制到位于中介位置的上传堆里
    //接着再通过调用ID3D12CommandList::CopySubresourceRegion函数,把上传堆内的数据复制到mBuffer中
    auto k = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &k);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    auto l = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &l);

    //注意:在调用上述函数后，必须保证uploadBuffer依然存在，而不能对它立即进行销毁。这是因为
    //命令列表中的复制操作可能尚未执行。待调用者得知复制完成的消息后，方可释放uploadBuffer



    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
    //若处于调试模式,则使用调试标志
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
    //在运行期间用下列函数对着色器进行编译
	hr = D3DCompileFromFile(
        filename.c_str(), 
        defines, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), //着色器的入口点函数名。一个.hlsl文件可能存有多个着色器程序（例如，一个顶点着色器和一个像素着色器)，所以我们需要为待编译的着色器指定入口点
        target.c_str(), //指定所用着色器类型和版本的字符串
        compileFlags, 
        0, 
        &byteCode, //返回一个指向ID3DBlob数据结构的指针，它存储着编译好的着色器对象字节码
        &errors);//返回一个指向ID3DBlob 数据结构的指针。如果在编译过程中发生了错误,它便会储存报错的字符串

    //将错误信息输出到调试窗口
	if(errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


