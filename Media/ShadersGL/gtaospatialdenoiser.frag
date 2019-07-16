
#version 330

uniform sampler2D gtao;

out vec2 my_FragColor0;

void main()
{
	// NOTE: 4x4 filter offsets image
	ivec2 loc = ivec2(gl_FragCoord.xy) - ivec2(2);
	float totalao = 0.0;

	// NOTE: textureGather requires GL 4
	totalao += texelFetch(gtao, loc, 0).r;
	totalao += texelFetch(gtao, loc + ivec2(1, 0), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(0, 1), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(1, 1), 0).r;

	totalao += texelFetch(gtao, loc + ivec2(2, 0), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(3, 0), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(2, 1), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(3, 1), 0).r;

	totalao += texelFetch(gtao, loc + ivec2(0, 2), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(1, 2), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(0, 3), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(1, 3), 0).r;

	totalao += texelFetch(gtao, loc + ivec2(2, 2), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(3, 2), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(2, 3), 0).r;
	totalao += texelFetch(gtao, loc + ivec2(3, 3), 0).r;

	my_FragColor0 = vec2(totalao / 16.0, 0.0);
}
