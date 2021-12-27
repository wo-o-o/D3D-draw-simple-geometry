//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************


//隐式填充为256B
cbuffer cbPerObject : register(b0)
{
	//4×4矩阵 gWorldViewProj，表示把一个点从局部空间变换到齐次裁剪空间所用到的由世界、视图和投影3种变换组合而成的矩阵。
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
	
	// 将顶点变换到齐次裁剪空间
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// 直接把顶点颜色传递到像素着色器
    vout.Color = vin.Color;
    
    return vout;
}

//像素着色器返回一个4D 颜色值，位于此函数参数列表后的Sv_TARGET 语义
//表示该返回值的类型应当与渲染目标格式( render target format)相匹配(该输出值会被存于渲染目标之中 )。
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}


