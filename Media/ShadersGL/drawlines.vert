
#version 430

uniform mat4 matProj;
uniform int numControlVertices;

// NOTE: this is patch data!!! (0, 1, 1, 2, 2, 3, 3, ..., n)
layout(std140, binding = 0) readonly buffer ControlVertexData {
	vec4 data[];
} controlVertices;

layout(std140, binding = 1) readonly buffer TangentData {
	vec4 data[];
} tangents;

void main()
{
	// this is a cool trick to generate lines from point data
	vec4 linevert;

	if (gl_VertexID % 2 == 0) {
		// use control point
		int index = min(gl_VertexID, numControlVertices - 1);
		vec3 pos = controlVertices.data[index].xyz;

		linevert = matProj * vec4(pos, 1.0);
	} else {
		// use tangent
		int index = min(gl_VertexID - 1, numControlVertices - 1);
		vec3 pos = controlVertices.data[index].xyz;
		vec3 tang = tangents.data[gl_VertexID / 2].xyz;

		linevert = matProj * vec4(pos + tang, 1.0);
	}

	gl_Position = linevert;
}
