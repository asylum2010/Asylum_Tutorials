
#version 440

layout(location = 4, binding = 0) uniform sampler3D sampler0;
layout(location = 5, binding = 1) uniform sampler2D sampler1;

in vec3 tex;

layout(location = 0) out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = texture(sampler0, tex) * texture(sampler1, vec2(tex.z, 0.5));
}