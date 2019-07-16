
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

float2 ComputeParallaxOffset(float2 dp, float2 ds, int numsteps)
{
	float stepsize	= 1.0f / (float)numsteps;
	float curr		= 0.0f;				// current height
	float prev		= 1.0f;				// previous height
	float bound		= 1.0f;				// upper bound

	float2 offstep	= stepsize * ds;	// step size along ds
	float2 offset	= dp;				// current offset along ds
	float2 pt1		= 0.0f;
	float2 pt2		= 0.0f;

	int i = 0;

	// try to "box in" the point we are looking for
	[unroll]
	while (i < numsteps) {
		// step in the heightfield
		offset += offstep;
		curr = tex2D(normalMap, offset).w;

		// and on y
		bound -= stepsize;

		// test whether we are below the heightfield
		if (bound < curr) {
			pt1 = float2(bound, curr);
			pt2 = float2(bound + stepsize, prev);

			// exit loop
			i = numsteps + 1;
		} else {
			++i;
			prev = curr;
		}
	}

	// calculate intersection
	float d2 = pt2.x - pt2.y;
	float d1 = pt1.x - pt1.y;

	float d = d2 - d1;
	float amount = 0.0f;

	if (d != 0.0f)
		amount = (pt1.x * d2 - pt2.x * d1) / d;

	return dp + ds * (1.0f - amount);
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
	float2 searchdir = normalize(float2(-vdir_ts.x, vdir_ts.y));

	// calculate maximum offset (using trigonometry)
	float vdir_l = length(vdir_ts);
	float maxoffset = sqrt(vdir_l * vdir_l - vdir_ts.z * vdir_ts.z) / vdir_ts.z;

	// calculate correct texcoord (limit height)
	float2 dp = tex;
	float2 ds = searchdir * maxoffset * 0.2f;

	tex = ComputeParallaxOffset(dp, ds, 15);

	// calculate light direction
	float3 vpos = vdir;
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

	color.rgb = base * lightAmbient + (f_diffuse + f_specular) * lightColor * ndotl;
	color.a = base.a;
}

technique parallaxocclusion
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
