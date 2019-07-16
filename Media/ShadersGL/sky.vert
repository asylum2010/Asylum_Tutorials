
#version 330

in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;
uniform vec3 eyePos;

out vec3 view;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1.0);
	view = wpos.xyz - eyePos;

	gl_Position = matViewProj * wpos;
}
