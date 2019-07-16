
#version 330

uniform sampler2D sampler0;
uniform vec2 texelSize;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	const int radius = 3;

	vec4 color = vec4(0.0);

	for (int i = -radius; i <= radius; ++i) {
		for (int j = -radius; j <= radius; ++j) {
			color += texture(sampler0, tex + vec2(i, j) * texelSize);
		}
	}

	color /= (4.0 * float(radius * radius + radius) + 1.0);
	my_FragColor0 = color;
}
