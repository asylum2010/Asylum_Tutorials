
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
uniform float heightLimit	= 0.2f;

float RayIntersectHeightfield(float2 dp, float2 ds)
{
	const int linear_search_steps = 15;
	const int binary_search_steps = 15;

	float4 t;
	float size			= 1.0f / linear_search_steps;
	float depth			= 0.0f;	// where we are
	float best_depth	= 1.0f;	// best estimate so far

	// find first point inside heightfield
	[unroll]
	for (int i = 0; i < linear_search_steps - 1; ++i) {
		depth += size;
		t = tex2D(normalMap, dp + ds * depth);

		if (best_depth > 0.996f) {
			if (depth >= (1.0f - t.w))
				best_depth = depth;
		}
	}

	depth = best_depth;

	// look for exact intersection
	[unroll]
	for (int i = 0; i < binary_search_steps; ++i) {
		size *= 0.5f;
		t = tex2D(normalMap, dp + ds * depth);

		if (depth >= (1.0f - t.w)) {
			best_depth = depth;
			depth -= 2.0f * size;
		}

		depth += size;
	}

	return best_depth;
}

float CalculateShadow(float3 l, float2 tex)
{
	// NOTE: from ATI's paper
	const float softy = 0.58f;
	
	l.xy *= heightLimit;
	l.y = -l.y;

	float sh0 = tex2D(normalMap, tex).w;

	// shi = 1 * (Db - Ds) * (1 / Dlb')
	float shA = (tex2D(normalMap, tex + l.xy * 0.88f).w - sh0) * 1 * softy;
	float sh9 = (tex2D(normalMap, tex + l.xy * 0.77f).w - sh0) * 2 * softy;
	float sh8 = (tex2D(normalMap, tex + l.xy * 0.66f).w - sh0) * 4 * softy;
	float sh7 = (tex2D(normalMap, tex + l.xy * 0.55f).w - sh0) * 6 * softy;
	float sh6 = (tex2D(normalMap, tex + l.xy * 0.44f).w - sh0) * 8 * softy;
	float sh5 = (tex2D(normalMap, tex + l.xy * 0.33f).w - sh0) * 10 * softy;
	float sh4 = (tex2D(normalMap, tex + l.xy * 0.22f).w - sh0) * 12 * softy;

	float shadow = 1 - max(max(max(max(max(max(shA, sh9), sh8), sh7), sh6), sh5), sh4);
	shadow = shadow * 0.4f + 0.6f;

	return shadow;
}

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wtan		: TEXCOORD1,
	out		float3 wbin		: TEXCOORD2,
	out		float3 wnorm	: TEXCOORD3,
	out		float3 vdir		: TEXCOORD4)
{
	pos = mul(float4(pos.xyz, 1), matWorld);

	wnorm = normalize(mul(matWorldInv, float4(norm, 0)).xyz);
	wtan = normalize(mul(float4(tang, 0), matWorld).xyz);
	wbin = normalize(mul(float4(bin, 0), matWorld).xyz);

	vdir = pos.xyz - eyePos.xyz;

	pos = mul(pos, matViewProj);
	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wtan		: TEXCOORD1,
	in	float3 wbin		: TEXCOORD2,
	in	float3 wnorm	: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float3 v = normalize(vdir);
	float height = tex2D(normalMap, tex).w;

	// dv is flipped for some reason
	float3x3 tbn = { wtan, -wbin, wnorm };

	// determine search direction
	float3 vdir_ts = mul(tbn, v);
	float2 searchdir = float2(-vdir_ts.x, vdir_ts.y) / vdir_ts.z;

	// find intersection (limit height)
	float2 dp = tex;
	float2 ds = searchdir * heightLimit;

	float t = RayIntersectHeightfield(dp, ds);
	tex = dp + ds * t;

	// calculate light direction
	float3 vpos = vdir + (v * t * vdir_ts.z);
	float3 ldir = normalize(lightPos.xyz - vpos);

	float3 l = normalize(mul(tbn, ldir));
	float3 n = tex2D(normalMap, tex).xyz * 2.0f - 1.0f;
	float3 h = normalize(l - vdir_ts);

	// do lighting
	float4 base = tex2D(baseColor, tex);

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	float3 f_diffuse = base;
	float f_specular = pow(ndoth, 80);

	float shadow = CalculateShadow(l, tex);

	color.rgb = base * lightAmbient + (f_diffuse + f_specular) * shadow * lightColor * ndotl;
	color.a = base.a;
}

technique relief
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
