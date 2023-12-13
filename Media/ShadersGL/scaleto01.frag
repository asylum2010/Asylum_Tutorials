
#version 330

uniform sampler2D sampler0;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	float value = texture(sampler0, tex).x;
	value = value * 0.5 + 0.5;

	my_FragColor0 = vec4(value, value, value, 1.0);
}
