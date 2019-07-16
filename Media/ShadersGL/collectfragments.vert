
#version 430

in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matView;
uniform mat4 matProj;

out vec4 vpos;

void main()
{
	vpos = matView * (matWorld * vec4(my_Position, 1));
	gl_Position = matProj * vpos;
}
