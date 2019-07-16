
#include <metal_stdlib>

using namespace metal;

struct LightParticle
{
	float4 Color;
	float4 Previous;
	float4 Current;
	float4 Velocity;	// w = radius
};

struct ConstantBuffer
{
	float4x4 matView;
	float4x4 matProj;
	float4 params;
};

struct VS_OUTPUT
{
	float4 pos [[position]];
	float2 tex;
	float4 color;
};

vertex VS_OUTPUT vs_flares(
	const device LightParticle* lightbuffer [[buffer(0)]],
	constant ConstantBuffer& uniforms [[buffer(1)]],
	uint vid [[vertex_id]],
	uint iid [[instance_id]])
{
	VS_OUTPUT out;
	
	const float4 positions[4] = {
		float4(-1, 1, 0, 1),
		float4(1, 1, 0, 1),
		float4(-1, -1, 0, 1),
		float4(1, -1, 0, 1)
	};

	const float2 texcoords[4] = {
		float2(0, 0),
		float2(1, 0),
		float2(0, 1),
		float2(1, 1)
	};

	float4 pos = mix(lightbuffer[iid].Previous, lightbuffer[iid].Current, uniforms.params.x);
	float4 vpos = uniforms.matView * pos;

	float psin = sin(uniforms.params.y * 6.5) * 0.5 + 0.5;
	float scale = mix(0.1, 0.25, psin);
	
	float theta = sin(uniforms.params.y * 0.25) * 3.0;
	float cosa = cos(theta);
	float sina = sin(theta);
	
	float3x3 rot = float3x3(
		float3(cosa, -sina, 0.0),
		float3(sina, cosa, 0.0),
		float3(0.0, 0.0, 1.0));
	
	float3 scaled = positions[vid].xyz * float3(scale, scale, 1.0);
	float3 rotated = rot * scaled;
	
	out.tex = texcoords[vid];
	out.color = lightbuffer[iid].Color;
	out.pos = uniforms.matProj * float4(rotated + vpos.xyz, 1.0);
	
	return out;
}

fragment half4 ps_flares(
	texture2d<half> tex0 [[texture(0)]],
	VS_OUTPUT in [[stage_in]])
{
	constexpr sampler sampler0(coord::normalized, address::clamp_to_edge, filter::linear);
	
	half4 base = tex0.sample(sampler0, in.tex);
	return mix(base, base * half4(in.color), 0.75);
}
