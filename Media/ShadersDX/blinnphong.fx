
#define ONE_OVER_PI	0.318309f

uniform sampler2D baseColor : register(s0);

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
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2,
	out		float3 vdir		: TEXCOORD3)
{
	pos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	ldir = lightPos.xyz - pos.xyz;
	vdir = eyePos.xyz - pos.xyz;

	pos = mul(pos, matViewProj);
	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	in	float3 vdir		: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm);

	float4 base = tex2D(baseColor, tex);

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	// Lambert BRDF (game version; see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/)
	float3 f_diffuse = /*ONE_OVER_PI **/ base.rgb;

	// Blinn-Phong BRDF (needs larger exponent than Phong)
	float f_specular = pow(ndoth, 80);

	color.rgb = base.rgb * lightAmbient + (f_diffuse + f_specular) * lightColor * ndotl;
	color.a = base.a;
}

technique blinnphong
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
