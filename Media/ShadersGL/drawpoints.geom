
#version 430

uniform vec2 pointSize;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

void main()
{
	vec4 pos = gl_in[0].gl_Position;

	gl_Position = pos + vec4(-pointSize.x, pointSize.y, 0.0, 0.0) * pos.w;
	EmitVertex();

	gl_Position = pos + vec4(-pointSize.x, -pointSize.y, 0.0, 0.0) * pos.w;
	EmitVertex();

	gl_Position = pos + vec4(pointSize.x, pointSize.y, 0.0, 0.0) * pos.w;
	EmitVertex();

	gl_Position = pos + vec4(pointSize.x, -pointSize.y, 0.0, 0.0) * pos.w;
	EmitVertex();
}
