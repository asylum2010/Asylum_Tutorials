
#version 330

out vec4 my_FragColor0;

uniform float smoothingLevel;

in vec2 uv;
in vec4 linecolor;
in float linethickness;
in float linelength;

void main()
{
	vec2 t = uv;

	float ext_outer = linethickness * 0.5 + smoothingLevel;
	float ext_inner = linethickness * 0.5 - smoothingLevel;
	float d = abs(uv.y) - ext_inner;

	t.x = uv.x * (linelength + 2.0 * ext_outer) - ext_outer;

	if (t.x < 0.0)
		d = length(t) - ext_inner;
	else if (t.x >= linelength)
		d = length(t - vec2(linelength, 0.0)) - ext_inner;

	if (d < 0.0) {
		// inside line
		my_FragColor0 = vec4(linecolor.xyz, 1.0);
	} else {
		// edge to be smoothed
		d /= smoothingLevel;
		my_FragColor0 = vec4(linecolor.xyz, exp(-d * d));
	}
}
