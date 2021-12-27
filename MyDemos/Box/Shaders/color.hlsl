//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************


//��ʽ���Ϊ256B
cbuffer cbPerObject : register(b0)
{
	//4��4���� gWorldViewProj����ʾ��һ����Ӿֲ��ռ�任����βü��ռ����õ��������硢��ͼ��ͶӰ3�ֱ任��϶��ɵľ���
	float4x4 gWorldViewProj; 
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// ������任����βü��ռ�
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// ֱ�ӰѶ�����ɫ���ݵ�������ɫ��
    vout.Color = vin.Color;
    
    return vout;
}

//������ɫ������һ��4D ��ɫֵ��λ�ڴ˺��������б���Sv_TARGET ����
//��ʾ�÷���ֵ������Ӧ������ȾĿ���ʽ( render target format)��ƥ��(�����ֵ�ᱻ������ȾĿ��֮�� )��
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}


