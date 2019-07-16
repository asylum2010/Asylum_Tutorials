
#define QUAD_PI		12.566370614359172

uniform sampler2D shadowMap			: register(s2);
uniform samplerCUBE cubeShadowMap	: register(s3);

uniform matrix lightViewProj;

uniform float4 eyePos;
uniform float4 lightPos;
uniform float4 spotDirection	= { 0, 0, 1, 0 };
uniform float4 spotParams		= { 0, 0, 0, 0 };
uniform float2 clipPlanes;

uniform float lightFlux			= 10.0f;	// lumen
uniform float lightIlluminance	= 1.5f;		// lux
uniform float lightRadius		= 5.0f;		// meter

float AngleAttenuation(float3 l, float3 d)
{
	// x = cos(inner), y = cos(outer)
	float scale = 1.0f / max(1e-3f, spotParams.x - spotParams.y);
	float offset = -spotParams.y * scale;

	float cosa = -dot(l, d);
	float atten = saturate(cosa * scale + offset);

	return atten * atten;
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

float2 Illuminance_Blinn_Directional(float3 wpos, float4 wnorm)
{
	// the sun has an angular diameter between [0.526, 0.545] degrees
	const float sinAngularRadius = 0.0046251;
	const float cosAngularRadius = 0.9999893;

	float3 v = normalize(eyePos.xyz - wpos);
	float3 n = normalize(wnorm.xyz);

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

	float f_specular = pow(ndoth, wnorm.w);

	float costheta = saturate(dot(n, D));
	float illuminance = lightIlluminance * costheta;

	// calculate shadow (assumes ortho projection)
	float4 lspos = mul(float4(wpos, 1), lightViewProj);
	
	d = lspos.z;

	float2 ptex = (lspos.xy / lspos.w) * float2(0.5f, -0.5f) + 0.5f;
	float2 moments = tex2D(shadowMap, ptex).rg;
	float shadow = ShadowVariance(moments, d);

	return float2(illuminance * shadow, f_specular);
}

float2 Illuminance_Blinn_Spot(float3 wpos, float4 wnorm)
{
	float3 ldir = lightPos.xyz - wpos;

	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm.xyz);
	float3 d = normalize(spotDirection.xyz);

	float dist2 = max(dot(ldir, ldir), 1e-4f);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	float f_specular = pow(ndoth, wnorm.w);

	// calculate shadow (TODO:)
	float illuminance = (lightFlux / (QUAD_PI * dist2)) * ndotl;
	float attenuation = max(0, 1 - sqrt(dist2) / lightRadius);

	attenuation *= AngleAttenuation(l, d);

	return float2(illuminance * attenuation, f_specular);
}

float2 Illuminance_Blinn_Point(float3 wpos, float4 wnorm)
{
	float3 ldir = lightPos.xyz - wpos;

	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm.xyz);

	float dist2 = max(dot(ldir, ldir), 1e-4f);
	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	float f_specular = pow(ndoth, wnorm.w);

	// calculate shadow
	float2 moments = texCUBE(cubeShadowMap, -l).xy;

	float z = length(ldir);
	float d = (z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	float shadow = ShadowVariance(moments, d);

	float illuminance = (lightFlux / (QUAD_PI * dist2)) * ndotl;
	float attenuation = max(0, 1 - sqrt(dist2) / lightRadius);

	return float2(illuminance * attenuation * shadow, f_specular);
}
