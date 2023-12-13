
#version 440

in vec3 my_Position;
in vec3 my_Normal;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matView;
uniform mat4 matViewInv;
uniform mat4 matViewProj;

out vec4 vpos;
out vec3 vnorm;

invariant precise gl_Position;

void main()
{
	vpos = matView * (matWorld * vec4(my_Position, 1.0));
	vnorm = ((vec4(my_Normal, 0.0) * matWorldInv) * matViewInv).xyz;

	gl_Position = matViewProj * (matWorld * vec4(my_Position, 1.0));
}
