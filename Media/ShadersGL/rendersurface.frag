
#version 430

uniform vec4 lightDir;
uniform vec4 outsideColor;
uniform vec4 insideColor;
uniform int isWireMode;

in vec3 wnorm;
in vec3 vdir;

out vec4 my_FragColor0;

void main()
{
	my_FragColor0 = vec4(1.0);

	if (isWireMode == 0) {
		vec3 n = normalize(wnorm);
		vec3 l = normalize(lightDir.xyz);
		vec3 v = normalize(vdir);
		vec3 h = normalize(v + l);
		vec4 base = outsideColor;

		float s = 1.0;
		float d = 0.0;

		if (!gl_FrontFacing) {
			n = -n;
			base = insideColor;
			s = 0.25;
		}

		s *= pow(clamp(dot(h, n), 0.0, 1.0), 80.0);
		d = clamp(dot(l, n), 0.0, 1.0);

		my_FragColor0.rgb = clamp(base.rgb * d + vec3(s), vec3(0.0), vec3(1.0));
		my_FragColor0.a = 1.0;
	}
}
