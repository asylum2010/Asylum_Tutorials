
#version 330

uniform vec2 lineThickness;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

void main()
{
	vec4 cpos1 = gl_in[0].gl_Position;
	vec4 cpos2 = gl_in[1].gl_Position;

	vec4 spos1 = cpos1 / cpos1.w;
	vec4 spos2 = cpos2 / cpos2.w;

	vec2 d = normalize(spos2.xy - spos1.xy);
	vec2 n = vec2(d.y, -d.x);

	vec4 v1 = spos1 + vec4(n * lineThickness, 0.0, 0.0);
	vec4 v2 = spos1 - vec4(n * lineThickness, 0.0, 0.0);
	vec4 v3 = spos2 + vec4(n * lineThickness, 0.0, 0.0);
	vec4 v4 = spos2 - vec4(n * lineThickness, 0.0, 0.0);

	gl_Position = v1 * cpos1.w;
	EmitVertex();

	gl_Position = v3 * cpos2.w;
	EmitVertex();

	gl_Position = v2 * cpos1.w;
	EmitVertex();

	gl_Position = v4 * cpos2.w;
	EmitVertex();
}
