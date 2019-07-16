
#define NUM_MIPS	8
#define ONE_OVER_PI	0.3183098861837906

uniform samplerCUBE irradiance1	: register(s0);	// (preintegrated) diffuse irradiance
uniform samplerCUBE irradiance2	: register(s1);	// (preintegrated) specular irradiance
uniform sampler2D brdfLUT		: register(s2);	// (preintegrated) BRDF lookup texture

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 eyePos;

uniform float4 baseColor = { 0.3f, 1.0f, 1.0f, 1.0f };
uniform float roughness;

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float3 wnorm	: TEXCOORD0,
	out		float3 vdir		: TEXCOORD1)
{
	pos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	vdir = eyePos.xyz - pos.xyz;
	pos = mul(pos, matViewProj);
}

void ps_main(
	in	float3 wnorm	: TEXCOORD0,
	in	float3 vdir		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float3 n = normalize(wnorm);
	float3 v = normalize(vdir);
	float3 r = reflect(-v, n);

	float ndotv				= max(dot(n, v), 0.0);
	float miplevel			= roughness * (NUM_MIPS - 1);

	float3 diff_irrad		= texCUBE(irradiance1, r).rgb;
	float3 spec_irrad		= texCUBElod(irradiance2, float4(r, miplevel)).rgb;
	float2 f0_scale_bias	= tex2D(brdfLUT, float2(ndotv, roughness)).rg;

	float3 brdf_diff		= diff_irrad * baseColor.rgb * ONE_OVER_PI;
	float3 brdf_spec		= spec_irrad * (0.04f * f0_scale_bias.x + f0_scale_bias.y);

	color.rgb = brdf_diff + brdf_spec;
	color.a = baseColor.a;
}

technique insulator
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
