
#version 330

uniform sampler2D sampler0;
uniform vec2 texelSize;
uniform int prevLevel;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	// see https://www.cl.cam.ac.uk/~rkm38/pdfs/tone_mapping.pdf
	const vec3 LUMINANCE_VECTOR = vec3(0.2125, 0.7154, 0.0721);

#if SAMPLE_MODE == 0
	// initial
	float logsum = 0.0;

	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(-0.5, -0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(-0.5, 0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(-0.5, 1.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(0.5, -0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(0.5, 0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(0.5, 1.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(1.5, -0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(1.5, 0.5) * texelSize).rgb, LUMINANCE_VECTOR));
	logsum += log(0.0001 + dot(texture(sampler0, tex + vec2(1.5, 1.5) * texelSize).rgb, LUMINANCE_VECTOR));

	logsum /= 9.0;

	my_FragColor0 = vec4(logsum, 0.0, 0.0, 1.0);
#endif

#if SAMPLE_MODE > 0
	// iterative
	ivec2 loc = ivec2(gl_FragCoord.xy);
	float sum = 0.0;

	sum += texelFetch(sampler0, loc * 4 + ivec2(0, 0), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(0, 1), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(0, 2), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(0, 3), prevLevel).r;

	sum += texelFetch(sampler0, loc * 4 + ivec2(1, 0), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(1, 1), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(1, 2), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(1, 3), prevLevel).r;

	sum += texelFetch(sampler0, loc * 4 + ivec2(2, 0), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(2, 1), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(2, 2), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(2, 3), prevLevel).r;

	sum += texelFetch(sampler0, loc * 4 + ivec2(3, 0), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(3, 1), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(3, 2), prevLevel).r;
	sum += texelFetch(sampler0, loc * 4 + ivec2(3, 3), prevLevel).r;

	sum *= 0.0625;

#if SAMPLE_MODE == 1
	my_FragColor0 = vec4(sum, 0.0, 0.0, 1.0);
#elif SAMPLE_MODE == 2
	// final
	my_FragColor0 = vec4(exp(sum), 0.0, 0.0, 1.0);
#endif

#endif
}
