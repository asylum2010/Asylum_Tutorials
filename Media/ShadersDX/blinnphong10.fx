
#define ONE_OVER_PI	0.318309f

uniform Texture2D baseColor;

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos		= { -10, 8, -5, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float3 lightAmbient	= { 0.01f, 0.01f, 0.01f };
uniform float4 eyePos;
uniform float2 uv			= { 1, 1 };

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

void vs_main(
	in		float3 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2,
	out		float3 vdir		: TEXCOORD3,
	out		float4 opos		: SV_Position)
{
	opos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	ldir = lightPos.xyz - opos.xyz;
	vdir = eyePos.xyz - opos.xyz;

	opos = mul(opos, matViewProj);
	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	in	float3 vdir		: TEXCOORD3,
	out	float4 color	: SV_Target)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm);

	float4 base = baseColor.Sample(linearSampler, tex);

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	// Lambert BRDF (game version; see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/)
	float3 f_diffuse = /*ONE_OVER_PI **/ base.rgb;

	// Blinn-Phong BRDF (needs larger exponent than Phong)
	float f_specular = pow(ndoth, 80);

	color.rgb = base.rgb * lightAmbient + (f_diffuse + f_specular) * lightColor * ndotl;
	color.a = base.a;
}

technique10 blinnphong
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
