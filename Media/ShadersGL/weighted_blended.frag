
#version 330

uniform vec4 matColor;

out vec4 my_FragColor0;
out vec4 my_FragColor1;

void main()
{
	float tmp = (1.0 - 0.99 * gl_FragCoord.z) * matColor.a * 10.0;
	float weight = clamp(tmp, 0.01, 30.0);

	my_FragColor0 = vec4(matColor.rgb * matColor.a, matColor.a) * weight;
	my_FragColor1 = vec4(matColor.a);
}
