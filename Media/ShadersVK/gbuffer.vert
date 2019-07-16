
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 my_Position;
layout (location = 1) in vec3 my_Normal;
layout (location = 2) in vec2 my_Texcoord0;
layout (location = 3) in vec3 my_Tangent;
layout (location = 4) in vec3 my_Bitangent;

layout (std140, binding = 0) uniform UniformData {
	mat4 matWorld;
	mat4 matWorldInv;
	mat4 matViewProj;
} uniforms;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec3 wnorm;
layout (location = 1) out vec3 wtan;
layout (location = 2) out vec3 wbin;
layout (location = 3) out vec2 tex;
layout (location = 4) out vec4 cpos;

void main()
{
	wnorm = (vec4(my_Normal, 0.0) * uniforms.matWorldInv).xyz;
	wtan = (uniforms.matWorld * vec4(my_Tangent, 0.0)).xyz;
	wbin = (uniforms.matWorld * vec4(my_Bitangent, 0.0)).xyz;

	tex = my_Texcoord0;
	cpos = uniforms.matViewProj * (uniforms.matWorld * vec4(my_Position, 1.0));

	gl_Position = cpos;
}
