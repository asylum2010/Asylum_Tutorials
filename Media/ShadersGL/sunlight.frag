
#version 440

#include "pbr_common.head"

uniform sampler2D albedo;
uniform sampler2D shadowMap;

uniform vec3 sunDir;
uniform vec2 texelSize;

in vec3 wnorm;
in vec3 vdir;
in vec2 tex;
in vec4 ltov;

out vec4 my_FragColor0;

void main()
{
	const float sinAngularRadius = 0.0046251;
	const float cosAngularRadius = 0.9999893;
	const float sunIlluminance = 20.0; //1000.0;
	const float shadowbias = 5e-3;

	const vec3 sunColor = vec3(1.0, 1.0, 0.72);
	const vec3 F0 = vec3(0.04);

	vec3 n = normalize(wnorm);
	vec3 v = normalize(vdir);

	// calculate shadow
	vec4 projpos = (ltov / ltov.w);
	float shadow = 0.0;
	float z, dz = projpos.z;

	projpos.xy = projpos.xy * 0.5 + vec2(0.5);

	// always sample from texel center
	projpos.xy += texelSize * 0.5;

	// 5x5 PCF
	for (int i = -2; i < 3; ++i) {
		for (int j = -2; j < 3; ++j) {
			z = texture(shadowMap, projpos.xy + vec2(i, j) * texelSize).x;
			z += shadowbias;

			shadow += ((z < dz) ? 0.0 : 1.0);
		}
	}

	shadow *= 0.04;

	// closest point to disk (approximation)
	float r = sinAngularRadius;
	float d = cosAngularRadius;

	vec3 R = reflect(-v, n);
	vec3 D = sunDir;

	float DdotR = dot(D, R);

	vec3 S = R - DdotR * D;
	vec3 L = ((DdotR < d) ? normalize(d * D + normalize(S) * r) : R);

	// pull it out a little so it hides projection aliasing
	float ndotL = max(dot(n, L) * 1.06 - 0.06, 0.0);

	// calculate luminance
	vec3 f_lambert = BRDF_Lambertian(albedo, tex).xyz;
	vec3 f_specular = BRDF_CookTorrance(L, v, n, F0, matParams.x);
	vec3 illuminance = sunColor * sunIlluminance * ndotL;

	vec3 luminance = (f_lambert + f_specular) * illuminance;

	my_FragColor0 = vec4(luminance * shadow, 1.0);
}
