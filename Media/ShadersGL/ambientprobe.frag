
#version 440

#include "pbr_common.head"

uniform sampler2D albedo;
uniform sampler2D aoTarget;
uniform samplerCube illumDiffuse;

in vec3 wnorm;
in vec2 tex;

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
	vec3 n = normalize(wnorm);

	float ao = texelFetch(aoTarget, loc, 0).r;

	vec3 f_lambert = BRDF_Lambertian_NoDivision(albedo, tex).xyz;
	vec3 diffuse_rad = texture(illumDiffuse, n).rgb * f_lambert.rgb;

	diffuse_rad = diffuse_rad * MultiBounce(ao, diffuse_rad);

	my_FragColor0 = vec4(diffuse_rad, 1.0);
}
