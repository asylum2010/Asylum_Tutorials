
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2D sampler1;

layout (location = 0) in vec2 tex;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 outColor;

void main()
{
	vec4 base = texture(sampler1, tex);
	outColor = mix(base, base * color, 0.75);
}
