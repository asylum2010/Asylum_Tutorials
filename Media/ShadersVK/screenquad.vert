
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (push_constant) uniform TransformData {
	mat4 matTexture;
} transform;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec2 tex;

void main()
{
	vec4 positions[4] = vec4[4](
		vec4(-1.0, -1.0, 0.0, 1.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, -1.0, 0.0, 1.0),
		vec4(1.0, 1.0, 0.0, 1.0));

	vec2 texcoords[4] = vec2[4](
		vec2(0.0, 0.0),
		vec2(0.0, 1.0),
		vec2(1.0, 0.0),
		vec2(1.0, 1.0));

	tex = (transform.matTexture * vec4(texcoords[gl_VertexIndex], 0.0, 1.0)).xy;
	gl_Position = positions[gl_VertexIndex];
}
