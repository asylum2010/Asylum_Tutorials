
uniform float2 lightSize	= { 0, 0 };	// in light space
uniform float2 clipPlanes	= { 0.1f, 10.0f };
uniform float2 texelSize;

static const float2 PoissonDisk[16] = {
	{ -0.94201624, -0.39906216 }, 
	{ 0.94558609, -0.76890725 }, 
	{ -0.094184101, -0.92938870 }, 
	{ 0.34495938, 0.29387760 }, 
	{ -0.91588581, 0.45771432 }, 
	{ -0.81544232, -0.87912464 }, 
	{ -0.38277543, 0.27676845 }, 
	{ 0.97484398, 0.75648379 }, 
	{ 0.44323325, -0.97511554 }, 
	{ 0.53742981, -0.47373420 }, 
	{ -0.26496911, -0.41893023 }, 
	{ 0.79197514, 0.19090188 }, 
	{ -0.24188840, 0.99706507 }, 
	{ -0.81409955, 0.91437590 }, 
	{ 0.19984126, 0.78641367 }, 
	{ 0.14383161, -0.14100790 }
};

static const float2 IrregKernel[8] = {
	{ -0.072, -0.516 },
	{ -0.105, 0.989 },
	{ 0.949, 0.258 },
	{ -0.966, 0.216 },
	{ 0.784, -0.601 },
	{ 0.139, 0.230 },
	{ -0.816, -0.516 },
	{ 0.529, 0.779 }
};

float2 FindAverageBlockerDepth(sampler2D shadowmap, float d, float viewz, float2 uv)
{
	// NOTE: from similarity of triangles
	float2 searchregion = lightSize * (viewz - clipPlanes.x) / viewz;
	float2 ret = { 0, 0 };

	for (int i = 0; i < 16; ++i) {
		float z = tex2D(shadowmap, uv + PoissonDisk[i] * searchregion).r;

		if (z < d) {
			ret.x += z;
			++ret.y;
		}
	}

	ret.x /= ret.y;
	return ret;
}

float GradientBasedBias(sampler2D shadowmap, float2 uv)
{
	// use finite differences
	float d0 = tex2D(shadowmap, uv + float2(-0.5f, 0) * texelSize).r;
	float d1 = tex2D(shadowmap, uv + float2(0.5f, 0) * texelSize).r;
	float d2 = tex2D(shadowmap, uv + float2(0, -0.5f) * texelSize).r;
	float d3 = tex2D(shadowmap, uv + float2(0, 0.5f) * texelSize).r;

	float du = abs(d1 - d3);
	float dv = abs(d0 - d2);

	return sqrt(du * du + dv * dv);
}

float Visibility_GSM(sampler2D shadowmap, float d, float2 uv)
{
	// gradient shadow mapping
	float gradient = GradientBasedBias(shadowmap, uv);
	float z = tex2D(shadowmap, uv).r;

	return saturate((z - d) / gradient + 1);
}

float Chebychev(float2 moments, float d)
{
	float mean		= moments.x;
	float variance	= max(moments.y - moments.x * moments.x, 1e-5f);
	float md		= mean - d;
	float pmax		= variance / (variance + md * md);

	pmax = smoothstep(0.1f, 1.0f, pmax);

	// NEVER forget that d > mean in Chebychev's inequality!!!
	return max(d <= mean, pmax);
}
