
#version 430

layout(location = 0) in vec3 my_Position;

void main()
{
	gl_Position = vec4(my_Position, 1.0);
}
