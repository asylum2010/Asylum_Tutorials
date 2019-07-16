
#version 330

uniform FragmentUniformData {
	vec4 matColor;
} fsuniforms;

in vec3 wnorm;
in vec3 vdir;
in vec3 ldir;

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

	my_FragColor0.rgb = fsuniforms.matColor.rgb * d + vec3(s);
	my_FragColor0.a = fsuniforms.matColor.a;
}
