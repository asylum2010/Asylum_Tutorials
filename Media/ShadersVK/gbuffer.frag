
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D normalMap;

layout (location = 0) in vec3 wnorm;
layout (location = 1) in vec3 wtan;
layout (location = 2) in vec3 wbin;
layout (location = 3) in vec2 tex;
layout (location = 4) in vec4 cpos;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out float outDepth;

void main()
{
	vec3 tnorm = texture(normalMap, tex).xyz;
	tnorm = tnorm * 2.0 - vec3(1.0);

	vec3 t = normalize(wtan);
	vec3 b = normalize(wbin);
	vec3 n = normalize(wnorm);

	mat3 tbn = mat3(t, b, n);
	n = tbn * tnorm;

	float depth = cpos.z / cpos.w;
	
	outNormal = vec4(n, 1.0);
	outDepth = depth;
}
