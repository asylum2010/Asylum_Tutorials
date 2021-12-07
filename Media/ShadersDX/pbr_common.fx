
#define ONE_OVER_PI	0.3183098861837906
#define PI			3.1415926535897932
#define QUAD_PI		12.566370614359172
#define EPSILON		1e-5	// to avoid division with zero

float3 F_Schlick(float3 f0, float u)
{
	return f0 + (float3(1, 1, 1) - f0) * pow(1.0f - u + EPSILON, 5.0f);
}

float D_GGX(float ndoth, float roughness)
{
	// Disney's suggestion
	float a = roughness * roughness;
	float a2 = a * a;

	// optimized formula for the GPU
	float d = (ndoth * a2 - ndoth) * ndoth + 1.0;

	return a2 / (PI * d * d + EPSILON);
}

float G_Smith_Schlick(float ndotl, float ndotv, float roughness)
{
	// Disney's suggestion
	float a = roughness + 1.0;
	float k = a * a * 0.125;

	// shadowing/masking functions
	float G1_l = ndotl * (1 - k) + k;
	float G1_v = ndotv * (1 - k) + k;

	// could be optimized out due to Cook-Torrance's denominator
	return (ndotl / G1_l) * (ndotv / G1_v);
}

float G_Smith_Lightprobe(float ndotl, float ndotv, float roughness)
{
	// DICE's suggestion
	float a = roughness * 0.5f + 0.5f;
	float a2 = a * a * a * a;

	float lambda_v = (-1 + sqrt(a2 * (1 - ndotl * ndotl) / (ndotl * ndotl) + 1)) * 0.5f;
	float lambda_l = (-1 + sqrt(a2 * (1 - ndotv * ndotv) / (ndotv * ndotv) + 1)) * 0.5f;

	return rcp(1 + lambda_v + lambda_l);
}

uint ReverseBits32(uint bits)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);

	return bits;
}

float2 Random(float3 pixel, float seed)
{
	const float3 scale1 = { 12.9898f, 78.233f, 151.7182f };
	const float3 scale2 = { 63.7264f, 10.873f, 623.6736f };

	float2 xi;
	
	xi.x = frac(sin(dot(pixel + seed, scale1)) * 43758.5453f + seed);
	xi.y = frac(sin(dot(pixel + seed, scale2)) * 43758.5453f + seed);

	return xi;
}

float2 Hammersley(uint index, uint numsamples)
{
	// see http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	float x1 = frac(float(index) / float(numsamples));
	float x2 = float(ReverseBits32(index)) * 2.3283064365386963e-10;
	
	return float2(x1, x2);
}

float3 UniformSample(float2 xi)
{
	float phi = 2 * PI * xi.x;
	float costheta = xi.y;
	float sintheta = sqrt(1 - costheta * costheta);

	// PDF = 1 / (2*PI)
	return float3(sintheta * cos(phi), sintheta * sin(phi), costheta);
}

float3 CosineSample(float2 xi)
{
	float phi = 2 * PI * xi.x;
	float costheta = sqrt(xi.y);
	float sintheta = sqrt(1 - costheta * costheta);

	// PDF = cos(theta) / PI
	return float3(sintheta * cos(phi), sintheta * sin(phi), costheta);
}

float3 GGXSample(float2 xi, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float phi = 2 * PI * xi.x;
	float costheta = sqrt((1 - xi.y) / (1 + (a2 - 1) * xi.y));
	float sintheta = sqrt(1 - costheta * costheta);

	// PDF = D(h) * dot(n, h)
	return float3(sintheta * cos(phi), sintheta * sin(phi), costheta);
}

float3 TangentToWorld(float3 v, float3 n)
{
	float3 up = ((abs(n.z) < 0.999f) ? float3(0, 0, 1) : float3(1, 0, 0));
	float3 t = normalize(cross(up, n));
	float3 b = cross(n, t);

	return t * v.x + b * v.y + n * v.z;
}

float2 BRDF_BlinnPhong(float3 wnorm, float3 ldir, float3 vdir, float shininess)
{
	float3 n = normalize(wnorm);
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float2 irrad;

	irrad.x = saturate(dot(n, l));
	irrad.y = pow(saturate(dot(h, n)), shininess);

	return irrad;
}
