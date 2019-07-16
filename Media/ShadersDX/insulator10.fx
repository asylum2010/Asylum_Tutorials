
#define NUM_MIPS	8
#define ONE_OVER_PI	0.3183098861837906

uniform TextureCube irradiance1;	// (preintegrated) diffuse irradiance
uniform TextureCube irradiance2;	// (preintegrated) specular irradiance
uniform Texture2D brdfLUT;			// (preintegrated) BRDF lookup texture

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 eyePos;

uniform float4 baseColor = { 0.3f, 1.0f, 1.0f, 1.0f };
uniform float roughness;

SamplerState noMipmapSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

void vs_main(
	in		float3 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float3 wnorm	: TEXCOORD0,
	out		float3 vdir		: TEXCOORD1,
	out		float4 opos		: SV_Position)
{
	opos = mul(float4(pos, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	vdir = eyePos.xyz - opos.xyz;
	opos = mul(opos, matViewProj);
}

void ps_main(
	in	float3 wnorm	: TEXCOORD0,
	in	float3 vdir		: TEXCOORD1,
	out	float4 color	: SV_Target)
{
	float3 n = normalize(wnorm);
	float3 v = normalize(vdir);
	float3 r = reflect(-v, n);

	float ndotv				= max(dot(n, v), 0.0);
	float miplevel			= roughness * (NUM_MIPS - 1);

	float3 diff_irrad		= irradiance1.Sample(noMipmapSampler, r).rgb;
	float3 spec_irrad		= irradiance2.SampleLevel(linearSampler, r, miplevel).rgb;
	float2 f0_scale_bias	= brdfLUT.Sample(linearSampler, float2(ndotv, roughness)).rg;

	float3 brdf_diff		= diff_irrad * baseColor.rgb * ONE_OVER_PI;
	float3 brdf_spec		= spec_irrad * (0.04f * f0_scale_bias.x + f0_scale_bias.y);

	color.rgb = brdf_diff + brdf_spec;
	color.a = baseColor.a;
}

technique10 insulator
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
