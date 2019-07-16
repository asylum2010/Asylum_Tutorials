
#version 330

in vec3 my_Position;
in vec2 my_Texcoord0;

uniform mat4 matWorld;
uniform mat4 matViewProj;
uniform vec2 uv;

out vec2 tex;

void main()
{
	tex = my_Texcoord0 * uv;
	gl_Position = matViewProj * (matWorld * vec4(my_Position, 1));
}
