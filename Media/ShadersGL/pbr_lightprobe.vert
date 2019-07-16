
#version 330

in vec3 my_Position;
in vec3 my_Normal;
in vec2 my_Texcoord0;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;

uniform vec3 eyePos;

out vec2 tex;
out vec3 wnorm;
out vec3 vdir;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1.0);

	vdir = eyePos - wpos.xyz;
	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;

	tex = my_Texcoord0;
	gl_Position = matViewProj * wpos;
}
