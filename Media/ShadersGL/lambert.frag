
#version 150

uniform vec4 matAmbient;
uniform vec4 matDiffuse;

#ifdef TEXTURED
uniform sampler2D sampler0;

in vec2 tex;
#endif

in vec3 wnorm;
in vec3 ldir;

out vec4 my_FragColor0;

void main()
{
	vec3 n = normalize(wnorm);
	vec3 l = normalize(ldir);

	float d = clamp(dot(l, n), 0.0, 1.0);

#ifdef TEXTURED
	vec4 base = texture(sampler0, tex);

	my_FragColor0.rgb = base.rgb * d + matAmbient.rgb;
	my_FragColor0.a = base.a;
#else
	my_FragColor0.rgb = matDiffuse.rgb * d + matAmbient.rgb;
	my_FragColor0.a = matDiffuse.a;
#endif
}
