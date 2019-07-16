
#include <metal_stdlib>

#define ONE_OVER_PI		0.318309

using namespace metal;

struct LightParticle
{
	float4 Color;
	float4 Previous;
	float4 Current;
	float4 Velocity;	// w = radius
};

struct ListHead
{
	uint4 StartAndCount;
};

struct ListNode
{
	uint4 LightIndexAndNext;
};

struct TBNVertex
{
	float3 pos [[attribute(0)]];
	float3 norm [[attribute(1)]];
	float2 tex [[attribute(2)]];
	float3 tangent [[attribute(3)]];
	float3 bitangent [[attribute(4)]];
};

struct VertexUniformData
{
	float4x4 matWorld;
	float4x4 matViewProj;
	float4 eyePos;
};

struct FragmentUniformData
{
	uint2 numTiles;
	float alpha;
};

static_assert(sizeof(FragmentUniformData) == 16, "sizeof(FragmentUniformData) must be 16 bytes");

struct VS_OUTPUT
{
	float4 pos [[position]];
	float4 wpos;
	float3 wtan;
	float3 wbin;
	float3 wnorm;
	float3 vdir;
	float2 tex;
};

vertex VS_OUTPUT vs_lightaccum(
	TBNVertex vert [[stage_in]],
	constant VertexUniformData& uniforms [[buffer(1)]])
{
	VS_OUTPUT out;
	
	out.wpos = uniforms.matWorld * float4(vert.pos, 1.0);
	
	out.wtan = (uniforms.matWorld * float4(vert.tangent, 0.0)).xyz;
	out.wbin = (uniforms.matWorld * float4(vert.bitangent, 0.0)).xyz;
	out.wnorm = (uniforms.matWorld * float4(vert.norm, 0.0)).xyz;
	
	out.vdir = uniforms.eyePos.xyz - out.wpos.xyz;
	out.tex = vert.tex;
	out.pos = uniforms.matViewProj * out.wpos;
	
	return out;
}

fragment half4 ps_lightaccum(
	VS_OUTPUT in [[stage_in]],
	texture2d<half> tex0 [[texture(0)]],
	texture2d<half> tex1 [[texture(1)]],
	texturecube<half> tex2 [[texture(2)]],
	const device ListHead* headbuffer [[buffer(0)]],
	const device ListNode* nodebuffer [[buffer(1)]],
	const device LightParticle* lightbuffer [[buffer(2)]],
	constant FragmentUniformData& uniforms [[buffer(3)]])
{
	constexpr sampler sampler0(coord::normalized, address::repeat, filter::linear, mip_filter::linear);
	constexpr sampler sampler1(coord::normalized, address::clamp_to_edge, filter::linear, mip_filter::linear);
	
	// NOTE: luminuous flux / QUAD_PI
	const float lumIntensity = 80.0f;
	
	uint2 	loc 		= uint2(in.pos.xy);
	uint2	tileID 		= loc / uint2(16, 16);
	int		index 		= tileID.y * uniforms.numTiles.x + tileID.x;
	
	float4	base 		= float4(tex0.sample(sampler0, in.tex));
	float3	tnorm 		= float3(tex1.sample(sampler0, in.tex).xyz) * 2.0f - 1.0f;
	float4 	pos;
	float3 	color;
	float	radius;

	float3	t = normalize(in.wtan);
	float3	b = normalize(in.wbin);
	float3 	n = normalize(in.wnorm);
	float3 	v = normalize(in.vdir);

	float3x3 tbn = float3x3(t, b, n);
	n = tbn * tnorm;
	
	uint 	start 		= headbuffer[index].StartAndCount.x;
	uint 	count 		= headbuffer[index].StartAndCount.y;
	uint 	nodeID 		= start;
	uint 	lightID;

	float3 	accumdiff 	= float3(tex2.sample(sampler1, n).rgb) * ONE_OVER_PI;
	float3 	accumspec 	= float3(0.0, 0.0, 0.0);
	
	for (uint i = 0; i < count; ++i) {
		lightID = nodebuffer[nodeID].LightIndexAndNext.x;
		nodeID 	= nodebuffer[nodeID].LightIndexAndNext.y;

		color 	= lightbuffer[lightID].Color.rgb;
		pos 	= mix(lightbuffer[lightID].Previous, lightbuffer[lightID].Current, uniforms.alpha);
		radius 	= lightbuffer[lightID].Velocity.w;

		float3 ldir		= pos.xyz - in.wpos.xyz;
		float3 l		= normalize(ldir);
		float3 h		= normalize(l + v);
		
		float dist		= length(ldir);
		float dist2		= max(dot(ldir, ldir), 1e-4f);
		float falloff	= (lumIntensity / dist2) * max(0.0, 1.0 - dist / radius);
		
		float costheta	= max(0.0, dot(n, l));
		float fs		= ONE_OVER_PI * pow(clamp(dot(n, h), 0.0, 1.0), 80.0);
		
		// add Lambertian term here
		accumdiff += color * ONE_OVER_PI * falloff * costheta;
		accumspec += color * fs * falloff * costheta;
	}

	float3 finalcolor = base.rgb * accumdiff + accumspec;
	return half4(half3(finalcolor), 1);
}
