
#version 430

#include "pbr_common.head"
#include "raytrace_common.head"

struct SceneObject
{
	vec4 params1;
	vec4 params2;
	vec4 color;
	ivec4 type;		// 1 -> plane, 2 -> sphere, 3 -> box, 4 -> cylinder
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

#ifdef RENDER_GBUFFER
uniform mat4	matView;
uniform mat4	matViewInv;
uniform vec2	clipPlanes;

out vec4 my_FragColor0;
out vec4 my_FragColor1;
out float my_FragColor2;
#else
out vec4 my_FragColor0;
#endif

int FindIntersection(out vec3 pos, out vec3 norm, vec3 raystart, vec3 raydir)
{
	vec3	bestn, n;
	float	t, bestt	= FLT_MAX;
	int		index		= numObjects;
	int		i;

	// find first object that the ray hits
	for (i = 0; i < numObjects; ++i) {
		SceneObject obj = objects.data[i];

		if (obj.type.x == 1)
			t = RayIntersectPlane(n, obj.params1, raystart, raydir);
		else if (obj.type.x == 2)
			t = RayIntersectSphere(n, obj.params1.xyz, obj.params1.w, raystart, raydir);
		else if (obj.type.x == 3)
			t = RayIntersectBox(n, obj.params1.xyz, obj.params2.xyz, raystart, raydir);
		else
			t = RayIntersectCylinder(n, obj.params1, obj.params2, raystart, raydir);

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

vec3 TraceScene(vec3 raystart, vec3 raydir)
{
	vec3	inray;
	vec3	outray;
	vec3	n, p, q;
	vec3	otherp;
	vec3	indirect = vec3(1.0);
	vec2	xi = Random2();
	int		index;

	p = raystart;
	outray = raydir;

	index = FindIntersection(p, n, p, raydir);

	if (index < numObjects) {
		inray = CosineSampleHemisphere(n, xi);
		index = FindIntersection(q, n, p, inray);

		if (index < numObjects)
			indirect = vec3(0.0);
	} else {
		return vec3(0.0, 0.0103, 0.0707);
	}

	return indirect;
}

void main()
{
	vec2 res	= vec2(textureSize(prevIteration, 0));
	vec3 spos	= gl_FragCoord.xyz;
	vec4 ndc	= vec4(tex * 2.0 - vec2(1.0), 0.1, 1.0);
	vec4 wpos	= matViewProjInv * ndc;
	vec3 raydir;

	// initialize RNG
	randomSeed = time + res.y * spos.x / res.x + spos.y / res.y;

	wpos /= wpos.w;
	raydir = normalize(wpos.xyz - eyePos);

#ifdef RENDER_GBUFFER
	vec3 p, n;
	int index = FindIntersection(p, n, eyePos, raydir);

	my_FragColor0 = vec4(0.0, 0.0, 0.0, 1.0);
	my_FragColor1 = vec4(0.0, 0.0, 0.0, 1.0);
	my_FragColor2 = 1.0;

	if (index < numObjects) {
		vec4 vpos = matView * vec4(p, 1.0);
		float depth = (-vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

		my_FragColor1.rgb = normalize((vec4(n, 0.0) * matViewInv).xyz);
		my_FragColor2 = depth;
	}
#else
	vec3 prev = texelFetch(prevIteration, ivec2(spos.xy), 0).rgb;
	vec3 curr = TraceScene(eyePos, raydir);
	float d = 1.0 / currSample;

	my_FragColor0.rgb = mix(prev, curr, d);
	my_FragColor0.a = 1.0;
#endif
}
