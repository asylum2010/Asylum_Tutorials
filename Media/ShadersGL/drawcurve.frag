
#version 430

layout(location = 3) uniform vec4 color;
layout(location = 0) out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = color;
}
