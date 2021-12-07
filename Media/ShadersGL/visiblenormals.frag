
#version 430

#include "raytrace_structs.head"
#include "pbr_common.head"
#include "raytrace_common.head"

layout (binding = 2) uniform sampler2DArray albedoTextures;
layout (binding = 3) uniform samplerCube environmentMap;

#include "bsdf_common.head"

#define MODEL_EPSILON	1e-3
#define MODEL_UVSCALE	5.0

struct SceneObject
{
	mat4 tounit;
	mat4 toworld;
	uvec4 params;
	BSDFInfo bsdf;
};

layout (std430, binding = 0) buffer ObjectBuffer {
	SceneObject data[];
} objects;

in vec2 tex;

uniform sampler2D prevIteration;

uniform mat4	matViewProjInv;
uniform vec3	eyePos;
uniform float	time;
uniform float	currSample;
uniform int		numObjects;

out vec4 my_FragColor0;

float IntersectionParameter(out vec3 norm, SceneObject obj, vec3 raystart, vec3 raydir)
{
	float t = FLT_MAX;
	uint shapetype = obj.params.x;

	if ((shapetype & SHAPE_PLATE) == SHAPE_PLATE)
		t = RayIntersectPlate(norm, obj.tounit, raystart, raydir);
	else if ((shapetype & SHAPE_SPHERE) == SHAPE_SPHERE)
		t = RayIntersectSphere(norm, obj.tounit, raystart, raydir);

	return t;
}

float IntersectionParameter(out vec3 norm, out vec2 uv, SceneObject obj, vec3 raystart, vec3 raydir)
{
	float t = FLT_MAX;
	uint shapetype = obj.params.x;

	uv = vec2(0.0);

	if ((shapetype & SHAPE_PLATE) == SHAPE_PLATE)
		t = RayIntersectPlate(norm, uv, obj.tounit, raystart, raydir);
	else if ((shapetype & SHAPE_SPHERE) == SHAPE_SPHERE)
		t = RayIntersectSphere(norm, obj.tounit, raystart, raydir);

	uv *= MODEL_UVSCALE;

	return t;
}

int FindIntersection(out vec3 pos, out vec3 norm, out vec2 uv, vec3 raystart, vec3 raydir)
{
	vec3	bestn;
	vec2	bestuv;
	float	t, bestt = FLT_MAX;
	int		index = numObjects;
	int		i;

	// find first object that the ray hits
	for (i = 0; i < numObjects; ++i) {
		SceneObject obj = objects.data[i];
		t = IntersectionParameter(norm, uv, obj, raystart, raydir);

		if (t < bestt) {
			bestt	= t;
			bestn	= norm;
			bestuv	= uv;
			index	= i;
		}
	}

	if (index < numObjects) {
		pos = raystart + bestt * raydir;
		norm = bestn;
		uv = bestuv;
	}

	return index;
}

float Visibility(vec3 raystart, vec3 raydir)
{
	vec3 n;
	vec3 p = raystart + 1e-3 * raydir;

	float t;
	float bestt = FLT_MAX;

	for (int i = 0; i < numObjects; ++i) {
		SceneObject obj = objects.data[i];
		t = IntersectionParameter(n, obj, p, raydir);

		if (t < bestt) {
			// found something closer
			return 0.0;
		}
	}

	return 1.0;
}

vec3 SampleLightExplicit(vec3 p, vec3 n, vec3 v, vec2 uv, SceneObject obj)
{
	vec4 sample1, sample2;
	vec3 sum = vec3(0.0);
	vec4 radiance;
	vec2 xi;

	float ndotl;
	float vis, weight;
	
	xi = Random2();

	// importance sample light
	sample1.xyz = UniformSampleSphere(xi);
	sample1.w = 1.0 / QUAD_PI;

	// test if the sample hits the light
	vis = Visibility(p, sample1.xyz);
	ndotl = vis * max(0.0, dot(sample1.xyz, n));

	radiance = texture(environmentMap, sample1.xyz);

	// evaluate BRDF (xyz = color, w = PDF)
	sample2 = EvaluateBSDF(obj.bsdf, sample1.xyz, v, n, uv);

	weight = PowerHeuristic(1, sample1.w, 1, sample2.w);
	sum += (radiance.rgb * sample2.xyz * ndotl * weight) * QUAD_PI;

	xi = Random2();

	// importance sample BRDF
	SampleEvaluateBSDFImportance(sample2, sample1.xyz, obj.bsdf, v, n, uv, xi);

	if (sample2.w < EPSILON)
		return sum;

	// test if the sample hits the light
	vis = Visibility(p, sample2.xyz);
	radiance = texture(environmentMap, sample2.xyz);

	// evaluate
	weight = PowerHeuristic(1, sample2.w, 1, sample1.w);
	sum += (radiance.rgb * sample1.xyz * vis * weight);

	return sum;
}

vec3 TraceScene(vec3 raystart, vec3 raydir)
{
	vec4 sample1;
	vec3 color = vec3(0.0);
	vec3 throughput = vec3(1.0);
	vec3 outray = raydir;
	vec3 n, p = raystart;
	vec3 indirect;
	vec2 uv;
	vec2 xi;
	
	int index;
	bool diracdelta = false;
	bool refracted = false;

	for (int bounce = 0; bounce < 5; ++bounce) {
		index = FindIntersection(p, n, uv, p, outray);

		if (index >= numObjects) {
			// hit light, don't double dip
			if (bounce == 0 || diracdelta || refracted) {
				// NOTE: caustics can't be handled this way
				color += throughput * texture(environmentMap, outray).rgb;
			}

			break;
		}

		SceneObject obj = objects.data[index];
		
		diracdelta = ((obj.bsdf.params.y & BSDF_SPECULAR) == BSDF_SPECULAR);

		// add some bias
		p -= MODEL_EPSILON * outray;

		if (!diracdelta) {
			// direct lighting
			color += throughput * SampleLightExplicit(p, n, -outray, uv, obj);
		}

		xi = Random2();

		// sample BSDF
		refracted = SampleEvaluateBSDFImportance(sample1, indirect, obj.bsdf, -outray, n, uv, xi);

		if (sample1.w < EPSILON || IsBlack(indirect))
			break;

		// flip the bias if the ray refracted
		if (refracted)
			p += 2.0 * MODEL_EPSILON * outray;

		throughput *= indirect;
		outray = sample1.xyz;

		// Russian roulette
		if (bounce > 3) {
			float q = max(throughput.x, max(throughput.y, throughput.z));

			if (xi.x > q) {
				break;
			}

			throughput /= q;
		}
	}

	return color;
}

void main()
{
	vec2 res	= vec2(textureSize(prevIteration, 0));
	vec4 ndc	= vec4(tex * 2.0 - vec2(1.0), 0.1, 1.0);
	vec4 wpos	= matViewProjInv * ndc;
	vec3 raydir;

	// initialize RNG
	randomSeed = time + res.y * gl_FragCoord.x / res.x + gl_FragCoord.y / res.y;

	wpos /= wpos.w;
	raydir = normalize(wpos.xyz - eyePos);

	vec3 prev = texelFetch(prevIteration, ivec2(gl_FragCoord.xy), 0).rgb;
	vec3 curr = TraceScene(eyePos, raydir);
	float d = 1.0 / currSample;

	my_FragColor0.rgb = mix(prev, curr, d);
	my_FragColor0.a = 1.0;
}
