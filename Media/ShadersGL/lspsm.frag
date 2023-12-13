
#version 440

in vec4 ltov;
out vec4 my_FragColor0;

void main()
{
	float z = (ltov.z / ltov.w);

	my_FragColor0 = vec4(z, z, z, 1.0);
}
