
#include "lighting_common.fx"

uniform sampler2D normals			: register(s0);
uniform sampler2D depth				: register(s1);

uniform sampler2D albedo			: register(s0);
uniform sampler2D diffIlluminance	: register(s1);
uniform sampler2D specIlluminance	: register(s2);
uniform samplerCUBE environment		: register(s3);

// for light pre-pass
uniform matrix matViewProjInv;

uniform float3 lightColor		= { 1, 1, 1 };
uniform float2 pixelSize;
uniform float2 texelSize;

// for forward pass
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 baseColor		= { 1, 1, 1, 1 };
uniform float2 uv				= { 1, 1 };

uniform float hasTexture		= 1.0f;
uniform float metalness			= 0.0f;

void vs_deferred(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_deferred(
	in	float2 tex		: TEXCOORD0,
	in	float2 fcoord	: VPOS,
	out float4 outdiff	: COLOR0,
	out float4 outspec	: COLOR1)
{
	float2 vtex		= fcoord * texelSize;
	float4 wnorm	= tex2D(normals, vtex);
	float d			= tex2D(depth, vtex).r;
	float4 wpos		= float4(tex.x * 2 - 1, 1 - 2 * tex.y, d, 1);

	wnorm.xyz = wnorm.xyz * 2.0f - 1.0f;
	wnorm.w *= 1000.0f;

	if (d > 0.0f) {
		// NOTE: x = illuminance, y = specular BRDF
		float2 result;

		wpos = mul(wpos, matViewProjInv);
		wpos /= wpos.w;

		if (lightPos.w < 0.5f) {
			// directional light
			result = Illuminance_Blinn_Directional(wpos.xyz, wnorm);
		} else if (lightPos.w < 0.8f) {
			// spot light
			result = Illuminance_Blinn_Spot(wpos.xyz, wnorm);
		} else {
			// point light
			result = Illuminance_Blinn_Point(wpos.xyz, wnorm);
		}

		outdiff.rgb = lightColor * result.x;
		outspec.rgb = lightColor * result.x * result.y;

		outdiff.a = outspec.a = 1.0f;
	} else {
		outdiff = float4(0, 0, 0, 0);
		outspec = float4(0, 0, 0, 0);
	}
}

void vs_postforward(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 vdir		: TEXCOORD2)
{
	float4 wpos = mul(float4(pos.xyz, 1), matWorld);
	
	pos = mul(wpos, matViewProj);
	vdir = wpos.xyz - eyePos.xyz;

	// be lazy
	wnorm = mul(float4(norm, 0), matWorld).xyz;
}

void ps_postforward(
	in	float2 tex		: TEXCOORD0,
	in	float2 fcoord	: VPOS,
	out	float4 color	: COLOR0)
{
	float2 vtex = fcoord * texelSize;

	float4 base = tex2D(albedo, tex);
	float4 diff_illum = tex2D(diffIlluminance, vtex);
	float4 spec_illum = tex2D(specIlluminance, vtex);

	base = lerp(baseColor, base, hasTexture);

	color = base * diff_illum + spec_illum;
	color.a = 1.0f;
}

void ps_postreflective(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 vdir		: TEXCOORD2,
	in	float2 fcoord	: VPOS,
	out	float4 color	: COLOR0)
{
	float2 vtex = fcoord * texelSize;
	
	float3 n = normalize(wnorm);
	float3 v = normalize(vdir);
	float3 r = reflect(v, n);

	float ndotv = saturate(-dot(n, v));

	float4 base = tex2D(albedo, tex);
	float4 diff_illum = tex2D(diffIlluminance, vtex);
	float4 spec_illum = tex2D(specIlluminance, vtex);
	float4 refl = texCUBE(environment, r);

	base = lerp(baseColor, base, hasTexture);

	float3 f_diffuse = lerp(base.rgb, 0, metalness);
	float3 F0 = lerp(0.04f, base.rgb, metalness);
	float3 F = F0 + (1.0f - F0) * pow(1.0f - ndotv + 1e-5f, 5.0f);

	color.rgb = f_diffuse * diff_illum + (refl + spec_illum.rgb) * F;
	color.a = 1.0f;
}

technique deferred
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_deferred();
		pixelshader = compile ps_3_0 ps_deferred();
	}
}

technique postforward
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_postforward();
		pixelshader = compile ps_3_0 ps_postforward();
	}
}

technique postreflective
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_postforward();
		pixelshader = compile ps_3_0 ps_postreflective();
	}
}
