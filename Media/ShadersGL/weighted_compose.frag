
#version 330

uniform sampler2D sumColors;
uniform sampler2D sumWeights;

out vec4 my_FragColor0;

void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy);

	vec4 sumcolor = texelFetch(sumColors, loc, 0);
	float sumweight = texelFetch(sumWeights, loc, 0).r;
	vec3 avgcolor = sumcolor.rgb / max(sumcolor.a, 0.00001);

	my_FragColor0 = vec4(avgcolor, sumweight);
}
