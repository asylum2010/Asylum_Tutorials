
#version 330

uniform sampler2D sampler0;
uniform vec4 matAmbient;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	vec4 base = texture(sampler0, tex);
	my_FragColor0 = base * matAmbient;
}
