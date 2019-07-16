
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 my_Position;

layout (std140, binding = 0) uniform UniformData {
	mat4 matViewProj;
	vec4 color;
} uniforms;

layout (push_constant) uniform TransformData {
	mat4 matWorld;
	vec4 extraParams;
} transform;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec4 color;

void main()
{
	vec4 wpos = transform.matWorld * vec4(my_Position, 1.0);
	color = uniforms.color * transform.extraParams;

	gl_Position = uniforms.matViewProj * wpos;
}
