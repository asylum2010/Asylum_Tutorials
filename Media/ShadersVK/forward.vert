
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 my_Position;
layout (location = 1) in vec3 my_Normal;
layout (location = 2) in vec2 my_Texcoord0;

layout (std140, binding = 0) uniform UniformData {
	mat4 matWorld;
	mat4 matViewProj;
} uniforms;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec2 tex;
layout (location = 1) out vec4 cpos;

void main()
{
	cpos = uniforms.matViewProj * (uniforms.matWorld * vec4(my_Position, 1));

	tex = my_Texcoord0;
	gl_Position = cpos;
}
