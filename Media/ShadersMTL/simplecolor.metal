
#include <metal_stdlib>

using namespace metal;

struct PositionVertex
{
	float4 pos [[attribute(0)]];
};

struct ConstantBuffer
{
	float4x4 matWorld;
	float4x4 matViewProj;
	float4 matColor;
};

struct VS_OUTPUT
{
	float4 pos [[position]];
	float4 color;
};

vertex VS_OUTPUT vs_simplecolor(
	PositionVertex vert [[stage_in]],
	constant ConstantBuffer& uniforms [[buffer(1)]])
{
	VS_OUTPUT out;
	
	float4 wpos = uniforms.matWorld * float4(vert.pos);
	
	out.pos = uniforms.matViewProj * wpos;
	out.color = uniforms.matColor;
	
	return out;
}

fragment half4 ps_simplecolor(
	VS_OUTPUT in [[stage_in]])
{
	return half4(in.color);
}
