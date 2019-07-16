
#define ONE_OVER_PI	0.318309f

uniform sampler baseColor : register(s0);
uniform sampler normalMap : register(s1);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos		= { -10, 8, -5, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float3 lightAmbient	= { 0.01f, 0.01f, 0.01f };
uniform float4 eyePos;
uniform float2 uv			= { 1, 1 };

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 ldir		: TEXCOORD1,
	out		float3 vdir		: TEXCOORD2)
{
	pos = mul(float4(pos.xyz, 1), matWorld);

	norm = normalize(mul(matWorldInv, float4(norm, 0)).xyz);
	tang = normalize(mul(float4(tang, 0), matWorld).xyz);
	bin = normalize(mul(float4(bin, 0), matWorld).xyz);

	ldir = lightPos.xyz - pos.xyz;
	vdir = eyePos.xyz - pos.xyz;

	// dv is flipped for some reason
	float3x3 tbn = { tang, -bin, norm };

	ldir = mul(tbn, ldir);
	vdir = mul(tbn, vdir);

	pos = mul(pos, matViewProj);
	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 ldir		: TEXCOORD1,
	in	float3 vdir		: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);

	float4 base = tex2D(baseColor, tex);
	float3 tnorm = tex2D(normalMap, tex).xyz;
	float3 n = tnorm * 2.0f - 1.0f;

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	// Lambert BRDF (game version; see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/)
	float3 f_diffuse = /*ONE_OVER_PI **/ base;

	// Blinn-Phong BRDF (needs larger exponent than Phong)
	float f_specular = pow(ndoth, 80);

	color.rgb = base * lightAmbient + (f_diffuse + f_specular) * lightColor * ndotl;
	color.a = base.a;
}

technique normalmapping
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
