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


//������Ҫ������Ϊ�н���ϴ�����������ʼ��Ĭ�ϻ�������D3D12_HEAP_TYPE_DEFAULT���е����ݣ�
//��˹������й��ߺ������Ա�����ÿ��ʹ��Ĭ�ϻ������ظ��ع�����
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // ����ʵ�ʵ�Ĭ�ϻ�������Դ
    auto g = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto h = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &g,
        D3D12_HEAP_FLAG_NONE,
        &h,
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // Ϊ�˽�CPU���ڴ��е����ݸ��Ƶ�Ĭ�ϻ�������
    //���ǻ���Ҫ����һ�������н�λ�õ��ϴ���
    auto i = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto j = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &i,
		D3D12_HEAP_FLAG_NONE,
        &j,
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // ��������ϣ�����Ƶ�Ĭ�ϻ������е�����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;//ָ��ĳ��ϵͳ�ڴ���ָ��,�����г�ʼ�����������õ����ݡ�
    subResourceData.RowPitch = byteSize;//���ڻ���������,�˲�����Ϊ���������ݵ��ֽ�����
    subResourceData.SlicePitch = subResourceData.RowPitch;//���ڻ��������ԣ��˲�����Ϊ���������ݵ��ֽ�����

    //�����ݸ��Ƶ�Ĭ�ϻ�������Դ������
    // UpdateSubresources�����������Ƚ����ݴ�cpu�˵��ڴ��и��Ƶ�λ���н�λ�õ��ϴ�����
    //������ͨ������ID3D12CommandList::CopySubresourceRegion����,���ϴ����ڵ����ݸ��Ƶ�mBuffer��
    auto k = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &k);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    auto l = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &l);

    //ע��:�ڵ������������󣬱��뱣֤uploadBuffer��Ȼ���ڣ������ܶ��������������١�������Ϊ
    //�����б��еĸ��Ʋ���������δִ�С��������ߵ�֪������ɵ���Ϣ�󣬷����ͷ�uploadBuffer



    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
    //�����ڵ���ģʽ,��ʹ�õ��Ա�־
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
    //�������ڼ������к�������ɫ�����б���
	hr = D3DCompileFromFile(
        filename.c_str(), 
        defines, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), //��ɫ������ڵ㺯������һ��.hlsl�ļ����ܴ��ж����ɫ���������磬һ��������ɫ����һ��������ɫ��)������������ҪΪ���������ɫ��ָ����ڵ�
        target.c_str(), //ָ��������ɫ�����ͺͰ汾���ַ���
        compileFlags, 
        0, 
        &byteCode, //����һ��ָ��ID3DBlob���ݽṹ��ָ�룬���洢�ű���õ���ɫ�������ֽ���
        &errors);//����һ��ָ��ID3DBlob ���ݽṹ��ָ�롣����ڱ�������з����˴���,����ᴢ�汨����ַ���

    //��������Ϣ��������Դ���
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


