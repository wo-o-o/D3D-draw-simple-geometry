#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "MathHelper.h"

extern const int gNumFrameResources;

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description) 
    #endif
#endif 		
    */

class d3dUtil
{
public:

    static bool IsKeyDown(int vkeyCode);

    static std::string ToString(HRESULT hr);

    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
		//常量缓冲区的大小必须是硬件最小分配空间（通常是256B）的整数倍
		//为此，要将其凑整为满足需求的最小的256的整数倍。我们现在通过输入值bytesize加上255, 再屏蔽求和结果的低2字节(即计算结果中小于256的数据部分）来实现这一点。

        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

//利用SubmeshGeometry来定义MeshGeometry中存储的单个几何体
//此结构体适用于将多个几何体数据存于一个顶点缓冲区和一个索引缓冲区的情况
//它提供了对存于顶点缓冲区和索引缓冲区中的单个几何体进行绘制所需的数据和偏移量
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	//通过此子网格来定义当前SubmeshGeometry结构体中所存几何体的包围盒(bounding box)。
	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	//指定此几何体网格集合的名称,这样我们就能根据此名找到它
	std::string Name;

	// 系统内存中的副本。由于顶点/索引可以是泛型格式(具体格式依用户而定)，所以用Blob类型来表示待用户在使用时再将其转换为适当的类型
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // 与缓冲区相关的数据
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	//一个MeshGeometry结构体能够存储一组顶点/索引缓冲区中的多个几何体
	//若利用下列容器来定义子网格几何体，我们就能单独地绘制出其中的子网格(单个几何体)
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    //为了将顶点缓冲区绑定到渲染流水线上，我们需要给这种资源创建一个顶点缓冲区视图（vertexbuffer view)。
    //与RTV不同的是，我们无须为顶点缓冲区视图创建描述符堆。
    //而且，顶点缓冲区视图是由D3D12_VERTEX_BUFFER_VIEW"结构体来表示。
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();//待创建视图的顶点缓冲区资源虚拟地址。可以通过ID3D12Resource::GetGPUVirtualAddress方法来获得此地址。
		vbv.StrideInBytes = VertexByteStride;//待创建视图的顶点缓冲区大小（用字节表示)。
		vbv.SizeInBytes = VertexBufferByteSize;//每个顶点元素所占用的字节数。

		return vbv;
	}

    //给索引缓冲区资源创建一个索引缓冲区视图(indexbuffer view),使索引缓冲区与渲染流水线绑定。
    //如同顶点缓冲区视图，无须为索引缓冲区视图创建描述符堆。由结构体D3D12_INDEX_BUFFER_VIEW来表示。
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();//待创建视图的索引缓冲区资源虚拟地址。调用ID3D12Resource ::GetGPUVirtualAddress方法来获取此地址。
		ibv.Format = IndexFormat;//索引的格式,索引的格式必须为表示16位索引的 DXGI_FORMAT_R16_UINT类型，或表示32位索引的DXGI_FORMAT_R32_UINT类型。(16位的索引可以减少内存和带宽的占用，但如果索引值范围超过了16位数据的表达范围，则也只能采用32位索引了。)
		ibv.SizeInBytes = IndexBufferByteSize;//待创建视图的索引缓冲区大小（以字节表示)。

		return ibv;
	}

	// 待数据上传至GPU后，我们就能释放这些内存了
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
    float FalloffStart = 1.0f;                          // point/spot light only
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
    float FalloffEnd = 10.0f;                           // point/spot light only
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
    float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

// Simple struct to represent a material for our demos.  A production 3D engine
// would likely create a class hierarchy of Materials.
struct Material
{
	// Unique material name for lookup.
	std::string Name;

	// Index into constant buffer corresponding to this material.
	int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	int NormalSrvHeapIndex = -1;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Material constant buffer data used for shading.
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = .25f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Texture
{
	// Unique material name for lookup.
	std::string Name;

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif