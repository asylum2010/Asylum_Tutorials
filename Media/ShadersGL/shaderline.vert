
#version 330

layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 segment0;
layout (location = 2) in vec2 segment1;
layout (location = 3) in vec4 color;
layout (location = 4) in float thickness;
layout (location = 5) in float totalLength;

uniform mat4 matProj;
uniform float smoothingLevel;

out vec2 uv;
out vec4 linecolor;
out float linethickness;
out float linelength;

void main()
{
	vec2 offsets = vec2(-1.0, 1.0);

	float direction = offsets[gl_VertexID % 2];
	float extrusion = thickness * 0.5 + smoothingLevel;

	vec2 n0 = normalize(vec2(-segment0.y, segment0.x));
	vec2 n1 = normalize(vec2(-segment1.y, segment1.x));
	vec2 n = n1;

	uv = vec2(pos.w / totalLength, direction * extrusion);

	if (pos.w > 0.0 && pos.w < totalLength) {
		// joint
		float costheta = dot(n0, n1);
		n = ((costheta < -0.999) ? n1 : normalize(n0 + n1));

		n /= dot(n, n1);
	}

	vec4 spos = vec4(pos.xy + direction * extrusion * n, 0.0, 1.0);

	linecolor = color;
	linethickness = thickness;
	linelength = totalLength;

	gl_Position = matProj * spos;
}
