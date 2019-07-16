
#define ONE_OVER_PI	0.318309f

uniform sampler baseColor : register(s0);
uniform sampler normalMap : register(s1);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos		= { -10, 8, -5, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float3 lightAmbient	= { 0.015f, 0.015f, 0.015f };
uniform float4 eyePos;
uniform float time = 0;

static const float2 wavedir1 = { -0.02f, 0 };
static const float2 wavedir2 = { 0, -0.013f };
static const float2 wavedir3 = { 0.007f, 0.007f };

// we want the projected position in [0, 1]
static const matrix matScale = {
	0.5f,	0,		0,		0.5f,
	0,		0.5f,	0,		0.5f, 
	0,		0,		1,		0,
	0,		0,		0,		1
};

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 ldir		: TEXCOORD1,
	out		float3 vdir		: TEXCOORD2,
	out		float4 projtex	: TEXCOORD3)
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
	projtex = mul(matScale, pos);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 ldir		: TEXCOORD1,
	in	float3 vdir		: TEXCOORD2,
	in	float4 projtex	: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);

	projtex.xy /= projtex.w;

	// get an average wave normal (similar to Gerstner waves)
	float3 n = 0;

	n += (tex2D(normalMap, tex + time * wavedir1).xyz * 2 - 1) * 0.3f;
	n += (tex2D(normalMap, tex + time * wavedir2).xyz * 2 - 1) * 0.3f;
	n += (tex2D(normalMap, tex + time * wavedir3).xyz * 2 - 1) * 0.4f;

	n = normalize(n);

	// calculate Fresnel term (Schlick)
	float F0 = 0.020018673;
	float F = F0 + (1.0 - F0) * pow(1.0 - dot(n, l), 5.0);

	// get cos(theta) and distribution
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	// Blinn-Phong BRDF
	float f_specular = pow(ndoth, 80);

	// add some turbulence from the view vector
	float3 r = reflect(-v, n);

	float3 refr = tex2D(baseColor, projtex.xy + v.xy * 0.02f).rgb;
	float3 refl = tex2D(baseColor, projtex.xy + r.xy * 0.02f).rgb;

	// always know which term your BRDF is!!!
	color.rgb = refr * lightAmbient + (lerp(refr, refl, F) + f_specular.xxx) * lightColor * ndotl;
	color.a = 1;
}

technique projectedwater
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
