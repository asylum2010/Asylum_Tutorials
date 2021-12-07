
#version 430

#include "raytrace_structs.head"
#include "pbr_common.head"
#include "raytrace_common.head"

layout (binding = 2) uniform sampler2DArray albedoTextures;

#include "bsdf_common.head"

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
		pos = raystart + (bestt - 1e-3) * raydir;
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
	vec3 center;
	vec3 lp, ln;
	vec2 xi;

	float radius, ndotl;
	float vis, weight;
	int index;

	for (int k = 0; k < numLights; ++k) {
		SceneObject light = objects.data[k];

		center = vec3(light.toworld[3].xyz);
		radius = light.toworld[0][0];

		xi = Random2();

		// importance sample light (xyz = direction, w = PDF)
		sample1 = ConeSampleSphere(p, center, radius, xi);

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

		center = vec3(light.toworld[3].xyz);
		radius = light.toworld[0][0];

		// evaluate light PDF
		sample1.w = PDF_ConeSampledSphere(p, center, radius);
	
		weight = PowerHeuristic(1, sample2.w, 1, sample1.w);
		sum += (light.bsdf.color.rgb * sample1.xyz * weight);
	}

	return sum;
}

vec3 TraceScene(vec3 raystart, vec3 raydir)
{
	vec3 p, n;
	int index = FindIntersection(p, n, raystart, raydir);

	if (index >= numObjects)
		return vec3(0.0);

	SceneObject obj = objects.data[index];
	uint shapetype = obj.params.x;

	if ((shapetype & SHAPE_EMITTER) == SHAPE_EMITTER) {
		// hit light
		return obj.bsdf.color.rgb;
	}

	// direct lighting only, no bounces
	return SampleLightsExplicit(p, n, -raydir, obj);
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
