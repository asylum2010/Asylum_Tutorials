
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (input_attachment_index = 0, binding = 0) uniform subpassInput input0;

layout (location = 0) in vec2 tex;
layout (location = 0) out vec4 outColor;

const float A = 0.22;
const float B = 0.30;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;

vec3 FilmicTonemap(vec3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void main()
{
	float exposure = 1.0;

	vec4 base = subpassLoad(input0);
	vec3 lincolor = FilmicTonemap(base.rgb * exposure);
	vec3 invlinwhite = 1.0 / FilmicTonemap(vec3(W));

	base.rgb = lincolor * invlinwhite;
	outColor = base;
}
