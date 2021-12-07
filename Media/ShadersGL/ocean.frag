
#version 430

#define ONE_OVER_4PI	0.0795774715459476

// NOTE: also defined in vertex shader
#define BLEND_START		8		// m
#define BLEND_END		200		// m

out vec4 my_FragColor0;

layout (binding = 1) uniform sampler2D perlin;
layout (binding = 2) uniform samplerCube envmap;
layout (binding = 3) uniform sampler2D gradients;

uniform vec4 uvParams;
uniform vec2 perlinOffset;
uniform vec3 oceanColor;

in vec3 vdir;
in vec2 tex;

void main()
{
	// NOTE: also defined in vertex shader
	const vec3 sunColor			= vec3(1.0, 1.0, 0.47);
	const vec3 perlinFrequency	= vec3(1.12, 0.59, 0.23);
	const vec3 perlinGradient	= vec3(0.014, 0.016, 0.022);
	const vec3 sundir			= vec3(0.603, 0.240, -0.761);

	// blend with Perlin waves
	float dist = length(vdir.xz);
	float factor = (BLEND_END - dist) / (BLEND_END - BLEND_START);
	vec2 perl = vec2(0.0);

	factor = clamp(factor * factor * factor, 0.0, 1.0);

	if (factor < 1.0) {
		vec2 ptex = tex + uvParams.zw;

		vec2 p0 = texture(perlin, ptex * perlinFrequency.x + perlinOffset).rg;
		vec2 p1 = texture(perlin, ptex * perlinFrequency.y + perlinOffset).rg;
		vec2 p2 = texture(perlin, ptex * perlinFrequency.z + perlinOffset).rg;

		perl = (p0 * perlinGradient.x + p1 * perlinGradient.y + p2 * perlinGradient.z);
	}

	// calculate thingies
	vec4 grad = texture(gradients, tex);
	grad.xy = mix(perl, grad.xy, factor);

	vec3 n = normalize(grad.xzy);
	vec3 v = normalize(vdir);
	vec3 l = reflect(-v, n);

	float F0 = 0.020018673;
	float F = F0 + (1.0 - F0) * pow(1.0 - dot(n, l), 5.0);

	vec3 refl = texture(envmap, l).rgb;

	// tweaked from ARM/Mali's sample
	float turbulence = max(1.6 - grad.w, 0.0);
	float color_mod = 1.0 + 3.0 * smoothstep(1.2, 1.8, turbulence);

	color_mod = mix(1.0, color_mod, factor);

#if 1
	// some additional specular (Ward model)
	const float rho = 0.3;
	const float ax = 0.2;
	const float ay = 0.1;

	vec3 h = sundir + v;
	vec3 x = cross(sundir, n);
	vec3 y = cross(x, n);

	float mult = (ONE_OVER_4PI * rho / (ax * ay * sqrt(max(1e-5, dot(sundir, n) * dot(v, n)))));
	float hdotx = dot(h, x) / ax;
	float hdoty = dot(h, y) / ay;
	float hdotn = dot(h, n);

	float spec = mult * exp(-((hdotx * hdotx) + (hdoty * hdoty)) / (hdotn * hdotn));
#else
	// modified Blinn-Phong model (cheaper)
	float spec = pow(clamp(dot(sundir, l), 0.0, 1.0), 400.0);
#endif

	my_FragColor0 = vec4(mix(oceanColor, refl * color_mod, F) + sunColor * spec, 1.0);
}
