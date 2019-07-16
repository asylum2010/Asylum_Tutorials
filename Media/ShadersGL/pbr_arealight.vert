
#version 330

in vec3 my_Position;
in vec3 my_Normal;
in vec2 my_Texcoord0;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;

uniform vec4 eyePos;

out vec4 wpos;
out vec3 wnorm;
out vec3 vdir;
out vec2 tex;

void main()
{
	wpos = matWorld * vec4(my_Position, 1.0);
	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;

	vdir = eyePos.xyz - wpos.xyz;
	tex = my_Texcoord0;

	gl_Position = matViewProj * wpos;
}
