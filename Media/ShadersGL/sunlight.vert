
#version 440

in vec3 my_Position;
in vec3 my_Normal;
in vec2 my_Texcoord0;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;
uniform mat4 lightViewProj;
uniform vec3 eyePos;
uniform vec2 uvScale;

out vec3 wnorm;
out vec3 vdir;
out vec2 tex;
invariant precise out vec4 ltov;

invariant precise gl_Position;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1.0);

	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;
	vdir = eyePos - wpos.xyz;
	tex = my_Texcoord0 * uvScale;

	ltov = lightViewProj * wpos;

	gl_Position = matViewProj * wpos;
}
