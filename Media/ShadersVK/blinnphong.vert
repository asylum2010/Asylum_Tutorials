
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 my_Position;
layout (location = 1) in vec3 my_Normal;
layout (location = 2) in vec2 my_Texcoord0;

layout (std140, binding = 0) uniform UniformData {
	mat4 matWorld;
	mat4 matWorldInv;
	mat4 matViewProj;
	vec4 lightPos;
	vec4 eyePos;
} uniforms;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec3 wnorm;
layout (location = 1) out vec3 vdir;
layout (location = 2) out vec3 ldir;
layout (location = 3) out vec2 tex;

void main()
{
	vec4 wpos = uniforms.matWorld * vec4(my_Position, 1.0);

	ldir = uniforms.lightPos.xyz - wpos.xyz;
	vdir = uniforms.eyePos.xyz - wpos.xyz;

	wnorm = (vec4(my_Normal, 0.0) * uniforms.matWorldInv).xyz;
	tex = my_Texcoord0;

	gl_Position = uniforms.matViewProj * wpos;
}
