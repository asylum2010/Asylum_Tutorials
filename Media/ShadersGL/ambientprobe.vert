
#version 440

in vec3 my_Position;
in vec3 my_Normal;
in vec2 my_Texcoord0;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;
uniform vec2 uvScale;

out vec3 wnorm;
out vec2 tex;

invariant precise gl_Position;

void main()
{
	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;
	tex = my_Texcoord0 * uvScale;

	gl_Position = matViewProj * (matWorld * vec4(my_Position, 1.0));
}
