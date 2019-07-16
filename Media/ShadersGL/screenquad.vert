
#version 330

uniform mat4 matTexture;

out vec2 tex;

void main()
{
	vec4 positions[4] = vec4[4](
		vec4(-1.0, -1.0, 0.0, 1.0),
		vec4(1.0, -1.0, 0.0, 1.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, 1.0, 0.0, 1.0));

	vec2 texcoords[4] = vec2[4](
		vec2(0.0, 0.0),
		vec2(1.0, 0.0),
		vec2(0.0, 1.0),
		vec2(1.0, 1.0));

	tex = (matTexture * vec4(texcoords[gl_VertexID], 0.0, 1.0)).xy;
	gl_Position = positions[gl_VertexID];
}
