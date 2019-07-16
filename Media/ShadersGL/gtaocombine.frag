
#version 330

uniform sampler2D sampler0;
uniform sampler2D sampler1;

uniform int renderMode;

out vec4 my_FragColor0;

vec3 MultiBounce(float ao, vec3 albedo)
{
	vec3 x = vec3(ao);

	vec3 a = 2.0404 * albedo - vec3(0.3324);
	vec3 b = -4.7951 * albedo + vec3(0.6417);
	vec3 c = 2.7552 * albedo + vec3(0.6903);

	return max(x, ((x * a + b) * x + c) * x);
}

void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy);

	vec4 base = texelFetch(sampler0, loc, 0);
	float ao = texelFetch(sampler1, loc, 0).r;

	if (renderMode == 1) {
		my_FragColor0 = base;
	} else if (renderMode == 2) {
		my_FragColor0.rgb = base.rgb * MultiBounce(ao, base.rgb);
		my_FragColor0.a = base.a;
	} else if (renderMode == 3) {
		my_FragColor0 = vec4(ao, ao, ao, 1.0);
	}
}
