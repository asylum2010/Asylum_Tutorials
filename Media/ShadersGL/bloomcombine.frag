
#version 330

uniform sampler2D sampler0;
uniform sampler2D sampler1;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	vec4 base = texture(sampler0, tex);
	vec4 bloom = texture(sampler1, tex) * 0.5;

	vec2 vtex = tex - vec2(0.5);
	float vignette = 1.0 - dot(vtex, vtex);

	my_FragColor0.rgb = (base.rgb + bloom.rgb) * vignette * vignette * vignette;
	my_FragColor0.a = 1.0;
}
