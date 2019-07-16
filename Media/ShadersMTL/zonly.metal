
#include <metal_stdlib>

using namespace metal;

struct PositionVertex
{
	float3 pos [[attribute(0)]];
};

struct ConstantBuffer
{
	float4x4 matWorld;
	float4x4 matViewProj;
};

struct VS_OUTPUT
{
	float4 pos [[position]];
};

vertex VS_OUTPUT vs_zonly(
	PositionVertex vert [[stage_in]],
	constant ConstantBuffer& uniforms [[buffer(1)]])
{
	VS_OUTPUT out;
	
	float4 wpos = uniforms.matWorld * float4(vert.pos, 1.0);
	out.pos = uniforms.matViewProj * wpos;
	
	return out;
}

fragment half4 ps_zonly(
	VS_OUTPUT in [[stage_in]])
{
	return half4(0, 0, 0, 1);
}
