
#define ONE_OVER_PI		0.318309f
#define ONE_OVER_4PI	0.0795774715459476

uniform sampler normalMap		: register(s0);
uniform sampler refractionMap	: register(s1);
uniform sampler reflectionMap	: register(s2);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos		= { -10, 8, -5, 1 };
uniform float3 lightColor	= { 1, 1, 1 };
uniform float3 lightAmbient	= { 0.01f, 0.01f, 0.01f };
uniform float4 eyePos;
uniform float2 uv			= { 1, 1 };
uniform float time			= 0;

static const float2 WaveDir1 = { -0.02, 0 };
static const float2 WaveDir2 = { 0, -0.013 };
static const float2 WaveDir3 = { 0.007, 0.007 };

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 ldir		: TEXCOORD1,
	out		float3 vdir		: TEXCOORD2,
	out		float4 cpos		: TEXCOORD3)
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

	cpos = pos;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 ldir		: TEXCOORD1,
	in	float3 vdir		: TEXCOORD2,
	in	float4 cpos		: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);

	float3 n = 0;
	float2 t1 = tex * 0.6f; // scale texcoord (don't know texel size)

	n += (tex2D(normalMap, t1 + time * WaveDir1).rgb * 2 - 1) * 0.3f;
	n += (tex2D(normalMap, t1 + time * WaveDir2).rgb * 2 - 1) * 0.3f;
	n += (tex2D(normalMap, t1 + time * WaveDir3).rgb * 2 - 1) * 0.4f;

	n = normalize(n);

	// sample refraction & reflection maps
	float2 ptex1 = (cpos.xy / cpos.w) * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
	float2 ptex2 = float2(ptex1.x, 1.0f - ptex1.y);

	float4 refr = tex2D(refractionMap, ptex1 + n.xy * 2e-2f);
	float4 refl = tex2D(reflectionMap, ptex2 + n.xy * 2e-2f);

	// Fresnel equations
	const float n1 = 1.000293f;	// air
	const float n2 = 1.333f;	// water

	float3 F0 = (n1 - n2) / (n1 + n2);
	F0 *= F0;

	float ndotv = saturate(dot(n, v));
	float3 F = F0 + (1.0f - F0) * pow(1.0f - ndotv + 1e-5f, 5.0f);

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	// Ward BRDF
	const float rho = 0.3f;
	const float ax = 0.2f;
	const float ay = 0.2f;

	float3 x = cross(l, n);
	float3 y = cross(x, n);

	float mult = (ONE_OVER_4PI * rho / (ax * ay * sqrt(max(1e-5, ndotl * ndotv))));
	float hdotx = dot(h, x) / ax;
	float hdoty = dot(h, y) / ay;

	float spec = mult * exp(-((hdotx * hdotx) + (hdoty * hdoty)) / (ndoth * ndoth));

	color.rgb = (refr * ndotl * (1 - F) + refl * F) + spec * lightColor;
	color.a = 1;
}

technique water
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
