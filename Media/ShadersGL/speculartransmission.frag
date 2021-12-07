
#version 430

#include "raytrace_structs.head"
#include "pbr_common.head"
#include "raytrace_common.head"

layout (binding = 2) uniform sampler2DArray albedoTextures;

#include "bsdf_common.head"

#define MODEL_EPSILON	1e-3

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
uniform int		numLights;

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

vec4 SampleAreaLight(SceneObject light, vec3 p, vec2 xi)
{
	vec4 ret = vec4(0.0);
	uint shapetype = light.params.x;

	if ((shapetype & SHAPE_PLATE) == SHAPE_PLATE) {
		vec3 t = light.toworld[0].xyz;
		vec3 b = light.toworld[2].xyz;
		vec3 o = light.toworld[3].xyz;

		// CONSOLIDATE: this is incorrect
		ret = UniformSampleRect(p, o, t, b, xi);
	} else if ((shapetype & SHAPE_SPHERE) == SHAPE_SPHERE) {
		vec3 center = vec3(light.toworld[3].xyz);
		float radius = light.toworld[0][0];

		ret = ConeSampleSphere(p, center, radius, xi);
	}

	return ret;
}

float PDF_AreaLight(SceneObject light, vec3 p, vec3 lp)
{
	float ret = 0.0;
	uint shapetype = light.params.x;

	if ((shapetype & SHAPE_PLATE) == SHAPE_PLATE) {
		vec3 t = light.toworld[0].xyz;
		vec3 b = light.toworld[2].xyz;
		vec3 o = light.toworld[3].xyz;

		
		// CONSOLIDATE: this is incorrect
		ret = PDF_UniformSampledRect(p - lp, t, b);
	} else if ((shapetype & SHAPE_SPHERE) == SHAPE_SPHERE) {
		vec3 center = vec3(light.toworld[3].xyz);
		float radius = light.toworld[0][0];

		ret = PDF_ConeSampledSphere(p, center, radius);
	}

	return ret;
}

int FindIntersection(out vec3 pos, out vec3 norm, vec3 raystart, vec3 raydir)
{
	vec3	bestn, n;
	float	t, bestt = FLT_MAX;
	int		index = numObjects;
	int		i;

	// find first object that the ray hits
	for (i = 0; i < numObjects; ++i) {
		SceneObject obj = objects.data[i];
		t = IntersectionParameter(n, obj, raystart, raydir);

		if (t < bestt) {
			bestt	= t;
			bestn	= n;
			index	= i;
		}
	}

	if (index < numObjects) {
		pos = raystart + bestt * raydir;
		norm = bestn;
	}

	return index;
}

float Visibility(vec3 raystart, vec3 raydir, int target)
{
	vec3 n;
	vec3 p = raystart + 1e-3 * raydir;

	float t;
	float bestt = IntersectionParameter(n, objects.data[target], p, raydir);

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

vec3 SampleLightsExplicit(vec3 p, vec3 n, vec3 v, SceneObject obj)
{
	vec4 sample1, sample2;
	vec3 sum = vec3(0.0);
	vec3 lp, ln;
	vec2 xi;

	float ndotl;
	float vis, weight;
	int index;

	for (int k = 0; k < numLights; ++k) {
		SceneObject light = objects.data[k];

		xi = Random2();

		// importance sample light (xyz = direction, w = PDF)
		sample1 = SampleAreaLight(light, p, xi);

		// cast shadow ray
		vis = Visibility(p, sample1.xyz, k);
		ndotl = vis * max(0.0, dot(sample1.xyz, n));

		// evaluate BRDF (xyz = color, w = PDF)
		sample2 = EvaluateBSDF(obj.bsdf, sample1.xyz, v, n, vec2(0.0));

		if (sample1.w > EPSILON) {
			weight = PowerHeuristic(1, sample1.w, 1, sample2.w);
			sum += (light.bsdf.color.rgb * sample2.xyz * ndotl * weight) / sample1.w;
		}
	}

	xi = Random2();

	// importance sample BRDF
	SampleEvaluateBSDFImportance(sample2, sample1.xyz, obj.bsdf, v, n, vec2(0.0), xi);

	if (sample2.w < EPSILON)
		return sum;

	// test if the sample hits any light
	index = FindIntersection(lp, ln, p, sample2.xyz);

	if (index < numLights) {
		SceneObject light = objects.data[index];

		// evaluate light PDF
		sample1.w = PDF_AreaLight(light, p, lp);

		if (sample1.w > EPSILON) {
			weight = PowerHeuristic(1, sample2.w, 1, sample1.w);
			sum += (light.bsdf.color.rgb * sample1.xyz * weight);
		}
	}

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
	vec2 xi;
	
	int index;
	bool diracdelta = false;
	bool refracted = false;

	for (int bounce = 0; bounce < 15; ++bounce) {
		index = FindIntersection(p, n, p, outray);

		if (index >= numObjects)
			break;

		SceneObject obj = objects.data[index];
		uint shapetype = obj.params.x;

		if ((shapetype & SHAPE_EMITTER) == SHAPE_EMITTER) {
			// hit light, don't double dip
			if (bounce == 0 || diracdelta || refracted) {
				color += throughput * obj.bsdf.color.rgb;
			}

			break;
		}

		diracdelta = ((obj.bsdf.params.y & BSDF_SPECULAR) == BSDF_SPECULAR);

		// add some bias
		p -= MODEL_EPSILON * outray;

		if (!diracdelta) {
			// direct lighting
			color += throughput * SampleLightsExplicit(p, n, -outray, obj);
		}

		xi = Random2();

		// sample BSDF
		refracted = SampleEvaluateBSDFImportance(sample1, indirect, obj.bsdf, -outray, n, vec2(0.0), xi);

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
