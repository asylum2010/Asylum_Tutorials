
#version 330

#include "pbr_common.head"
#include "shadow_common.head"

uniform sampler2D baseColorSamp;
uniform sampler2D shadowMap;

uniform vec2 clipPlanes;

uniform vec3 lightColor;
uniform vec3 lightDir;

uniform float angleScale;
uniform float angleOffset;
uniform float lumIntensity;
uniform float invRadius;

in vec2 tex;
in vec3 wnorm;
in vec3 vdir;
in vec3 ldir;
in vec4 ltov;

out vec4 my_FragColor0;

float GetAngleAttenuation(vec3 l)
{
	float cosa = -dot(l, lightDir);
	float atten = clamp(cosa * angleScale + angleOffset, 0.0, 1.0);

	return atten * atten;
}

void main()
{
	vec3 n = normalize(wnorm);
	vec3 v = normalize(vdir);
	vec3 l = normalize(ldir);

	float ndotl = clamp(dot(n, l), 0.0, 1.0);

	vec4 fd = BRDF_Lambertian(baseColorSamp, tex);
	
	vec3 F0 = mix(vec3(0.04), baseColor.rgb, matParams.y);
	vec3 fs = BRDF_CookTorrance(l, v, n, F0, matParams.x);

	float dist		= length(ldir);
	float dist2		= max(dot(ldir, ldir), 1e-4);
	float falloff	= (lumIntensity / dist2) * max(0.0, 1.0 - dist * invRadius);

	falloff *= GetAngleAttenuation(l);

	float shadow = falloff * VarianceShadow2D(shadowMap, ltov, clipPlanes);
	float fade = max(0.0, (fd.a - 0.75) * 4.0);

	shadow = mix(1.0, shadow, fade);

	vec3 final_color = (fd.rgb * fd.a + fs) * ndotl * shadow; // * lightColor

	my_FragColor0.rgb = final_color;
	my_FragColor0.a = fd.a;
}
