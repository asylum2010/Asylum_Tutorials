
#version 330

in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;

out vec4 ltov;

void main()
{
	ltov = matViewProj * (matWorld * vec4(my_Position, 1.0));
	gl_Position = ltov;
}
