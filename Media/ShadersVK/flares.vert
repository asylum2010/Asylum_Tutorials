
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct LightParticle {
	vec4 color;
	vec4 previous;
	vec4 current;
	vec4 velocity;	// w = radius
};

layout (std140, binding = 0) uniform UniformData {
	mat4 matView;
	mat4 matProj;
	vec4 params;
} uniforms;

layout (std140, binding = 1) readonly buffer LightBuffer {
	LightParticle data[];
} lightbuffer;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec2 tex;
layout (location = 1) out vec4 color;

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

	vec4 pos = mix(lightbuffer.data[gl_InstanceIndex].previous, lightbuffer.data[gl_InstanceIndex].current, uniforms.params.x);
	vec4 vpos = uniforms.matView * pos;

	float psin = sin(uniforms.params.y * 6.5) * 0.5 + 0.5;
	float scale = mix(0.1, 0.25, psin);

	float theta = sin(uniforms.params.y * 0.25) * 3.0;
	float cosa = cos(theta);
	float sina = sin(theta);

	mat3 rot = mat3(
		cosa, -sina, 0.0,
		sina, cosa, 0.0,
		0.0, 0.0, 1.0);

	vec3 scaled = positions[gl_VertexIndex].xyz * vec3(scale, -scale, 1.0);
	vec3 rotated = rot * scaled;

	tex = texcoords[gl_VertexIndex];
	color = lightbuffer.data[gl_InstanceIndex].color;

	gl_Position = uniforms.matProj * vec4(rotated + vpos.xyz, 1.0);
}
