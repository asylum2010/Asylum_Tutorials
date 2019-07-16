
#version 330

uniform sampler2D renderedScene;
uniform sampler2D averageLuminance;

in vec2 tex;
out vec4 my_FragColor0;

const float A = 0.22;
const float B = 0.30;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void main()
{
	float Lavg = texelFetch(averageLuminance, ivec2(0, 0), 0).r;
	float two_ad_EV = Lavg * (100.0 / 12.5);
	float exposure = 1.0 / (1.2 * two_ad_EV);

	vec4 base = texture(renderedScene, tex);
	vec3 lincolor = Uncharted2Tonemap(base.rgb * exposure);
	vec3 invlinwhite = 1.0 / Uncharted2Tonemap(vec3(W));

	base.rgb = lincolor * invlinwhite;

	my_FragColor0 = base;
}
