//���ϴ�����������ز�����װһ�£�ʹ�����������㡣
//ʵ�����ϴ���������Դ�Ĺ���������������������Դ��ӳ���ȡ��ӳ����������ṩ��CopyData���������»������ڵ��ض�Ԫ�ء�
//����Ҫͨ��CPU�޸��ϴ������������ݵ�ʱ�����磬���۲�������˱仯)�������ʹ��CopyData��
//��������ڸ������͵��ϴ���������������ֻ��Գ�����������

#pragma once

#include "d3dUtil.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : 
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);

        //�����������Ĵ�СΪ256B��������
        //Ӳ��ֻ�ܰ�m*256B��ƫ������n*256B�����ݳ��������ֹ�����鿴��������
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // 256��������
        // UINT   SizeInBytes;   // 256��������
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
        if(isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        //ֻҪ�����޸ĵ�ǰ����Դ,���Ǿ�����ȡ��ӳ��
        //���ǣ�����Դ��GPUʹ���ڼ䣬����ǧ�򲻿������Դ����д���������Ա��������ͬ������)

    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if(mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};