
#version 330

uniform sampler2D sampler0;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = texture(sampler0, tex);
}
