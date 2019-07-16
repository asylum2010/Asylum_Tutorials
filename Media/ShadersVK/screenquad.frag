
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D sampler0;

layout (location = 0) in vec2 tex;
layout (location = 0) out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = texture(sampler0, tex);
}
