
#version 330

#include "pbr_common.head"

#define NUM_MIPS	8	// specular mip levels

uniform sampler2D baseColorSamp;
uniform samplerCube illumDiffuse;
uniform samplerCube illumSpecular;
uniform sampler2D brdfLUT;

in vec2 tex;
in vec3 wnorm;
in vec3 vdir;

out vec4 my_FragColor0;

void main()
{
	vec3 n = normalize(wnorm);
	vec3 v = normalize(vdir);
	vec3 r = 2.0 * dot(v, n) * n - v;

	float ndotv			= clamp(dot(n, v), 0.0, 1.0);
	float miplevel		= matParams.x * (NUM_MIPS - 1);

	vec4 fd				= BRDF_Lambertian(baseColorSamp, tex);
	vec3 diffuse_rad	= texture(illumDiffuse, n).rgb * fd.rgb;
	vec3 specular_rad	= textureLod(illumSpecular, r, miplevel).rgb;
	vec2 f0_scale_bias	= texture(brdfLUT, vec2(ndotv, matParams.x)).rg;

	vec3 F0				= mix(vec3(0.04), baseColor.rgb, matParams.y);
	vec3 F				= F0 * f0_scale_bias.x + vec3(f0_scale_bias.y);

	my_FragColor0.rgb = diffuse_rad * fd.a + specular_rad * F;
	my_FragColor0.a = fd.a;
}
