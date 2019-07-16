
#include "pbr_common.fx"
#include "shadow_common.fx"

#define POS_EXPONENT	20.0f
#define NEG_EXPONENT	10.0f

uniform sampler2D baseColor	: register(s0);
uniform sampler2D shadowMap	: register(s1);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;
uniform matrix lightViewProj;

uniform float4 lightPos		= { -10, 8, -5, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float3 lightAmbient	= { 0.01f, 0.01f, 0.01f };
uniform float4 eyePos;
uniform float2 uv			= { 1, 1 };

uniform float specularPower	= 80.0f;

float2 WarpDepth(float d)
{
	float2 ret;

	// must scale to [-1, 1]
	d = d * 2.0f - 1.0f;

	ret.x = exp(POS_EXPONENT * d);
	ret.y = -1.0f / exp(NEG_EXPONENT * d);

	return ret;
}

void vs_shadowmap(
	in out	float4 pos		: POSITION,
	out		float4 cpos		: TEXCOORD0)
{
	cpos = mul(float4(pos.xyz, 1), matWorld);
	cpos = mul(cpos, lightViewProj);

	pos = cpos;
}

void ps_shadowmap(
	in	float4 cpos		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	// assume ortho projection
	float2 warped = WarpDepth(cpos.z);
	color = float4(warped, warped * warped);
}

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2,
	out		float3 vdir		: TEXCOORD3,
	out		float4 cpos		: TEXCOORD4)
{
	pos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	ldir = lightPos.xyz - pos.xyz;
	vdir = eyePos.xyz - pos.xyz;

	cpos = mul(pos, lightViewProj);
	pos = mul(pos, matViewProj);

	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	in	float3 vdir		: TEXCOORD3,
	in	float4 cpos		: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float4 base = tex2D(baseColor, tex);
	float2 brdf = BRDF_BlinnPhong(wnorm, ldir, vdir, specularPower);
	float2 ptex = cpos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	// move to pixel center (important!)
	ptex += 0.5f * texelSize;
	
	float2 moments1 = tex2D(shadowMap, ptex).rb;
	float2 moments2 = tex2D(shadowMap, ptex).ga;

	// assume ortho projection
	float2 warped = WarpDepth(cpos.z);

	float estimate1 = Chebychev(moments1, warped.x);
	float estimate2 = Chebychev(moments2, warped.y);

	float shadow = min(estimate1, estimate2);

	color.rgb = base.rgb * lightAmbient + (base.rgb + brdf.yyy) * lightColor * brdf.x * shadow;
	color.a = base.a;
}

technique shadowmap
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_shadowmap();
		pixelshader = compile ps_3_0 ps_shadowmap();
	}
}

technique expvariance
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
