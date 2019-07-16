
#version 330

uniform samplerCube sampler0;

in vec3 view;

out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = texture(sampler0, view);
}
