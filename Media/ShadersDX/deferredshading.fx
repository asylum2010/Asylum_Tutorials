
#define QUAD_PI		12.566370614359172

uniform sampler2D albedo			: register(s0);
uniform sampler2D normals			: register(s1);
uniform sampler2D depth				: register(s2);
uniform sampler2D shadowMap			: register(s3);
uniform samplerCUBE cubeShadowMap	: register(s4);

uniform matrix matViewProjInv;
uniform matrix lightViewProj;

uniform float4 lightPos;
uniform float3 lightColor		= { 1, 1, 1 };
uniform float lightFlux			= 10.0f;	// lumen
uniform float lightIlluminance	= 1.5f;		// lux
uniform float lightRadius		= 5.0f;		// meter
uniform float specularPower		= 80.0f;

uniform float4 eyePos;
uniform float2 pixelSize;
uniform float2 clipPlanes;

void vs_main(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

float ShadowVariance(float2 moments, float d)
{
	float mean		= moments.x;
	float variance	= max(moments.y - moments.x * moments.x, 1e-5f);
	float md		= mean - d;
	float chebychev	= variance / (variance + md * md);

	chebychev = smoothstep(0.1f, 1.0f, chebychev);

	// NEVER forget that d > mean in Chebychev's inequality!!!
	return max(((d <= mean) ? 1.0f : 0.0f), chebychev);
}

float3 Luminance_Blinn_Directional(float3 albedo, float3 wpos, float3 wnorm)
{
	// the sun has an angular diameter between [0.526, 0.545] degrees
	const float sinAngularRadius = 0.0046251;
	const float cosAngularRadius = 0.9999893;

	float3 v = normalize(eyePos.xyz - wpos);
	float3 n = normalize(wnorm);

	// closest point to disk (approximation)
	float3 D = normalize(lightPos.xyz);
	float3 R = reflect(-v, n);

	float DdotR = dot(D, R);
	float r = sinAngularRadius;
	float d = cosAngularRadius;

	float3 S = R - DdotR * D;
	float3 L = ((DdotR < d) ? normalize(d * D + normalize(S) * r) : R);

	// BRDF (NOTE: should match values and exposure)
	float3 h = normalize(L + v);

	float ndotl = saturate(dot(n, L));
	float ndoth = saturate(dot(n, h));

	float3 f_diffuse = albedo;
	float f_specular = pow(ndoth, specularPower);

	float costheta = saturate(dot(n, D));
	float illuminance = lightIlluminance * costheta;

	// calculate shadow (assumes ortho projection)
	float4 lspos = mul(float4(wpos, 1), lightViewProj);
	
	d = lspos.z;

	float2 ptex = (lspos.xy / lspos.w) * float2(0.5f, -0.5f) + 0.5f;
	float2 moments = tex2D(shadowMap, ptex).rg;
	float shadow = ShadowVariance(moments, d);

	return (f_diffuse + f_specular) * lightColor * illuminance * shadow;
}

float3 Luminance_Blinn_Point(float3 albedo, float3 wpos, float3 wnorm)
{
	float3 ldir = lightPos.xyz - wpos;

	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm);

	float dist2 = max(dot(ldir, ldir), 1e-4f);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	float3 f_diffuse = albedo;
	float f_specular = pow(ndoth, specularPower);

	// calculate shadow
	float2 moments = texCUBE(cubeShadowMap, -l).xy;

	float z = length(ldir);
	float d = (z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	float shadow = ShadowVariance(moments, d);

	float illuminance = (lightFlux / (QUAD_PI * dist2)) * ndotl;
	float attenuation = max(0, 1 - sqrt(dist2) / lightRadius);

	return (f_diffuse + f_specular) * lightColor * illuminance * attenuation * shadow;
}

void ps_deferred(
	in	float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float4 base		= tex2D(albedo, tex);
	float3 wnorm	= tex2D(normals, tex).rgb * 2.0f - 1.0f;
	float d			= tex2D(depth, tex).r;
	float4 wpos		= float4(tex.x * 2 - 1, 1 - 2 * tex.y, d, 1);

	if (d > 0.0f) {
		wpos = mul(wpos, matViewProjInv);
		wpos /= wpos.w;

		if (lightPos.w < 0.5f) {
			// directional light
			color.rgb = Luminance_Blinn_Directional(base.rgb, wpos.xyz, wnorm);
		} else {
			// point light
			color.rgb = Luminance_Blinn_Point(base.rgb, wpos.xyz, wnorm);
		}

		color.a = 1;
	} else {
		color = float4(0, 0, 0, 0);
	}
}

technique deferred
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_deferred();
	}
}
