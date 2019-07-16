
#version 330

#include "shadow_common.head"

uniform sampler2D sampler0;
uniform sampler2D sampler1;		// shadowmap

uniform vec4 lightColor;
uniform vec4 matSpecular;
uniform vec2 clipPlanes;
uniform float ambient;

in vec4 ltov;
in vec3 wnorm;
in vec3 ldir;
in vec3 vdir;
in vec2 tex;

out vec4 my_FragColor0;

//layout(early_fragment_tests) in;
void main()
{
	vec3 n = normalize(wnorm);
	vec3 l = normalize(ldir);
	vec3 v = normalize(vdir);
	vec3 h = normalize(v + l);

	float diff = clamp(dot(l, n), 0.0, 1.0);
	float spec = pow(clamp(dot(h, n), 0.0, 1.0), 80.0);

	vec4 base = texture(sampler0, tex);
	float shadow = VarianceShadow2D(sampler1, ltov, clipPlanes);

	shadow = clamp(shadow + ambient, 0.0, 1.0);

	vec3 fd = base.rgb * diff * shadow;
	vec3 fs = matSpecular.rgb * spec * shadow;

	my_FragColor0.rgb = lightColor.rgb * (fd + fs);
	my_FragColor0.a = 1.0;
}
