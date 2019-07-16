
#include "pbr_common.fx"
#include "shadow_common.fx"

uniform sampler2D baseColor	: register(s0);
uniform sampler2D shadowMap	: register(s1);
uniform sampler2D noise		: register(s2);

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

static const float IrregKernelRadius = 2.0f;

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
	float d = cpos.z;
	color = float4(d, 0, 0, 1);
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
	out	float4 color	: COLOR0,
	uniform int mode)
{
	float4 base = tex2D(baseColor, tex);
	float2 brdf = BRDF_BlinnPhong(wnorm, ldir, vdir, specularPower);
	float2 ptex = cpos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	// move to pixel center (important!)
	ptex += 0.5f * texelSize;

	// assume ortho projection
	float d = cpos.z;
	float z;
	float shadow = 1.0f;

	if (mode == 0) {
		// unfiltered
		shadow = Visibility_GSM(shadowMap, d, ptex);

		//z = tex2D(shadowMap, ptex).r;
		
		//if (z < d)
		//	shadow = 0.0f;
	} else if (mode == 1) {
		// 5x5 PCF
		shadow = 0.0f;

		for (float i = -2.5f; i < 3.0f; ++i) {
			for (float j = -2.5f; j < 3.0f; ++j) {
				shadow += Visibility_GSM(shadowMap, d, ptex + float2(i, j) * texelSize);

				//z = tex2D(shadowMap, ptex + float2(i, j) * texelSize).r;
				//shadow += ((z < d) ? 0.0f : 1.0f);
			}
		}

		shadow *= 0.04f;
	} else if (mode == 2) {
		// irregular PCF
		shadow = 0.0f;

		float2 cos_sin = tex2D(noise, ptex * IrregKernelRadius * 15.0f).rg * 2.0f - 1.0f;
		float2 irreg_sample;

		float2x2 rotation = { cos_sin.x, -cos_sin.y, cos_sin.y, cos_sin.x };

		for( int i = 0; i < 8; ++i ) {
			irreg_sample = IrregKernel[i];
			irreg_sample = mul(irreg_sample, rotation) * IrregKernelRadius;

			shadow += Visibility_GSM(shadowMap, d, ptex + irreg_sample * texelSize);

			//z = tex2D(shadowMap, ptex + irreg_sample * texelSize).r;
			//shadow += ((z < d) ? 0.0f : 1.0f);
		}

		shadow *= 0.125f;
	} else if (mode == 3) {
		// PCSS
		// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf

		float viewz = clipPlanes.x + d * (clipPlanes.y - clipPlanes.x);
		float2 avgdepth = FindAverageBlockerDepth(shadowMap, d, viewz, ptex);

		if (avgdepth.y > 0.0f) {
			shadow = 0.0f;

			float avgz = clipPlanes.x + avgdepth.x * (clipPlanes.y - clipPlanes.x);

			float2 penumbra = lightSize * (viewz - avgz) / avgz;
			float2 filterradius = penumbra * clipPlanes.x / viewz;
			
			for (int i = 0; i < 16; ++i) {
				shadow += Visibility_GSM(shadowMap, d, ptex + PoissonDisk[i] * filterradius);

				//z = tex2D(shadowMap, ptex + PoissonDisk[i] * filterradius).r;
				//shadow += ((z < d) ? 0.0f : 1.0f);
			}

			shadow *= 0.0625f;
		}
	}

	// some filters might kick the result out of [0, 1]
	shadow = saturate(shadow);

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

technique unfiltered
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main(0);
	}
}

technique pcf5x5
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main(1);
	}
}

technique pcfirregular
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main(2);
	}
}

technique pcfsoft
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main(3);
	}
}
