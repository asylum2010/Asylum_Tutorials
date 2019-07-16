
#version 330

uniform sampler2D sampler0;
uniform sampler2D sampler1;
uniform float elapsedTime;

out vec4 my_FragColor0;

void main()
{
	float prevadaptation = texelFetch(sampler0, ivec2(0, 0), 0).r;
	float curravglum = texelFetch(sampler1, ivec2(0, 0), 6).r;
	float newadaptation = prevadaptation + (curravglum - prevadaptation) * (1.0 - pow(0.98, 50.0 * elapsedTime));

	my_FragColor0 = vec4(newadaptation, 0.0, 0.0, 1.0);
}
