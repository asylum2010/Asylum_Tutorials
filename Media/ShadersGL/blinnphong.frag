
#version 330

uniform vec4 matColor;

in vec3 wnorm;
in vec3 vdir;
in vec3 ldir;

#ifdef HARDWARE_INSTANCING
in vec4 instcolor;
#endif

out vec4 my_FragColor0;

void main()
{
	vec3 n = normalize(wnorm);
	vec3 l = normalize(ldir);
	vec3 v = normalize(vdir);
	vec3 h = normalize(v + l);

	float d = clamp(dot(l, n), 0.0, 1.0);
	float s = clamp(dot(h, n), 0.0, 1.0);

	s = pow(s, 80.0);

#ifdef HARDWARE_INSTANCING
	my_FragColor0.rgb = instcolor.rgb * matColor.rgb * d + vec3(s);
	my_FragColor0.a = instcolor.a * matColor.a;
#else
	my_FragColor0.rgb = matColor.rgb * d + vec3(s);
	my_FragColor0.a = matColor.a;
#endif
}
