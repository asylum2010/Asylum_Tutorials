
#include "lighting_common.fx"

uniform sampler2D albedo			: register(s0);
uniform sampler2D normalMap			: register(s1);
uniform samplerCUBE environment		: register(s4);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 baseColor	= { 1, 1, 1, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float2 uv			= { 1, 1 };

uniform float specularPower	= 80.0f;
uniform float hasTexture	= 1.0f;
uniform float metalness		= 0.0f;

void vs_forward(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 wpos		: TEXCOORD2)
{
	wpos = mul(float4(pos.xyz, 1), matWorld).xyz;
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	pos = mul(float4(wpos, 1), matViewProj);
	tex *= uv;
}

void ps_forwardreflective(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 wpos		: TEXCOORD2,
	out float4 color	: COLOR0)
{
	float3 n = normalize(wnorm);
	float3 v = normalize(wpos - eyePos.xyz);
	float3 r = reflect(v, n);

	float ndotv = saturate(-dot(n, v));

	float4 base = tex2D(albedo, tex);
	float4 refl = texCUBE(environment, r);
	float4 norms = float4(wnorm, specularPower);
	float2 result;

	if (lightPos.w < 0.5f) {
		// directional light
		result = Illuminance_Blinn_Directional(wpos.xyz, norms);
	} else if (lightPos.w < 0.8f) {
		// spot light
		result = Illuminance_Blinn_Spot(wpos.xyz, norms);
	} else {
		// point light
		result = Illuminance_Blinn_Point(wpos.xyz, norms);
	}

	float3 diff_illum = lightColor * result.x;
	float3 spec_illum = lightColor * result.x * result.y;

	base = lerp(baseColor, base, hasTexture);

	float3 f_diffuse = lerp(base.rgb, 0, metalness);
	float3 F0 = lerp(0.04f, base.rgb, metalness);
	float3 F = F0 + (1.0f - F0) * pow(1.0f - ndotv + 1e-5f, 5.0f);

	// NOTE: premultiplied alpha
	color.rgb = f_diffuse * diff_illum * base.a * (1 - F) + (refl + spec_illum.rgb) * F;
	color.a = base.a;
}

technique forwardreflective
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_forward();
		pixelshader = compile ps_3_0 ps_forwardreflective();
	}
}
