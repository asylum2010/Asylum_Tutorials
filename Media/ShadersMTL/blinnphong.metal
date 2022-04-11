
#include <metal_stdlib>

using namespace metal;

struct CommonVertex
{
	float3 pos [[attribute(0)]];
	float3 norm [[attribute(1)]];
	float2 tex [[attribute(2)]];
};

struct ConstantBuffer
{
	float4x4 matWorld;
	float4x4 matWorldInv;
	float4x4 matViewProj;
	float4 lightPos;
	float4 eyePos;
	float4 uvScale;
};

struct VS_OUTPUT
{
	float4 pos [[position]];
	float3 wnorm;
	float3 vdir;
	float3 ldir;
	float2 tex;
};

vertex VS_OUTPUT vs_blinnphong(
	CommonVertex vert [[stage_in]],
	constant ConstantBuffer& uniforms [[buffer(1)]])
{
	VS_OUTPUT out;
	
	float4 wpos = uniforms.matWorld * float4(vert.pos, 1.0);
	
	out.pos = uniforms.matViewProj * wpos;
	out.wnorm = (float4(vert.norm, 0.0) * uniforms.matWorldInv).xyz;
	out.vdir = uniforms.eyePos.xyz - wpos.xyz;
	out.ldir = uniforms.lightPos.xyz - wpos.xyz;
	out.tex = vert.tex * uniforms.uvScale.xy;
	
	return out;
}

fragment half4 ps_blinnphong(
	VS_OUTPUT in [[stage_in]],
	texture2d<half> tex0 [[texture(0)]])
{
	constexpr sampler sampler0(coord::normalized, address::repeat, filter::linear);
	
	float3 n = normalize(in.wnorm);
	float3 l = normalize(in.ldir);
	float3 v = normalize(in.vdir);
	float3 h = normalize(l + v);
	
	half d = saturate(dot(n, l));
	half s = pow(saturate(dot(n, h)), 80.0);

	half4 base = tex0.sample(sampler0, in.tex);
	base.rgb = base.rgb * d + half3(s);

	return base;
}
