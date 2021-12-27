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
		//�����������Ĵ�С������Ӳ����С����ռ䣨ͨ����256B����������
		//Ϊ�ˣ�Ҫ�������Ϊ�����������С��256������������������ͨ������ֵbytesize����255, ��������ͽ���ĵ�2�ֽ�(����������С��256�����ݲ��֣���ʵ����һ�㡣

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

//����SubmeshGeometry������MeshGeometry�д洢�ĵ���������
//�˽ṹ�������ڽ�������������ݴ���һ�����㻺������һ�����������������
//���ṩ�˶Դ��ڶ��㻺�����������������еĵ�����������л�����������ݺ�ƫ����
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	//ͨ���������������嵱ǰSubmeshGeometry�ṹ�������漸����İ�Χ��(bounding box)��
	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	//ָ���˼��������񼯺ϵ�����,�������Ǿ��ܸ��ݴ����ҵ���
	std::string Name;

	// ϵͳ�ڴ��еĸ��������ڶ���/���������Ƿ��͸�ʽ(�����ʽ���û�����)��������Blob��������ʾ���û���ʹ��ʱ�ٽ���ת��Ϊ�ʵ�������
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // �뻺������ص�����
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	//һ��MeshGeometry�ṹ���ܹ��洢һ�鶥��/�����������еĶ��������
	//�������������������������񼸺��壬���Ǿ��ܵ����ػ��Ƴ����е�������(����������)
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    //Ϊ�˽����㻺�����󶨵���Ⱦ��ˮ���ϣ�������Ҫ��������Դ����һ�����㻺������ͼ��vertexbuffer view)��
    //��RTV��ͬ���ǣ���������Ϊ���㻺������ͼ�����������ѡ�
    //���ң����㻺������ͼ����D3D12_VERTEX_BUFFER_VIEW"�ṹ������ʾ��
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();//��������ͼ�Ķ��㻺������Դ�����ַ������ͨ��ID3D12Resource::GetGPUVirtualAddress��������ô˵�ַ��
		vbv.StrideInBytes = VertexByteStride;//��������ͼ�Ķ��㻺������С�����ֽڱ�ʾ)��
		vbv.SizeInBytes = VertexBufferByteSize;//ÿ������Ԫ����ռ�õ��ֽ�����

		return vbv;
	}

    //��������������Դ����һ��������������ͼ(indexbuffer view),ʹ��������������Ⱦ��ˮ�߰󶨡�
    //��ͬ���㻺������ͼ������Ϊ������������ͼ�����������ѡ��ɽṹ��D3D12_INDEX_BUFFER_VIEW����ʾ��
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();//��������ͼ��������������Դ�����ַ������ID3D12Resource ::GetGPUVirtualAddress��������ȡ�˵�ַ��
		ibv.Format = IndexFormat;//�����ĸ�ʽ,�����ĸ�ʽ����Ϊ��ʾ16λ������ DXGI_FORMAT_R16_UINT���ͣ����ʾ32λ������DXGI_FORMAT_R32_UINT���͡�(16λ���������Լ����ڴ�ʹ����ռ�ã����������ֵ��Χ������16λ���ݵı�ﷶΧ����Ҳֻ�ܲ���32λ�����ˡ�)
		ibv.SizeInBytes = IndexBufferByteSize;//��������ͼ��������������С�����ֽڱ�ʾ)��

		return ibv;
	}

	// �������ϴ���GPU�����Ǿ����ͷ���Щ�ڴ���
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