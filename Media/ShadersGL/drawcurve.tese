
#version 430

uniform mat4 matProj;

layout(std140, binding = 0) readonly buffer TangentData {
	vec4 data[];
} tangents;

layout(isolines, equal_spacing) in;
void main()
{
	float u = gl_TessCoord.x;
	int patchID = gl_PrimitiveID;

	vec4 tan0 = tangents.data[patchID];
	vec4 tan1 = tangents.data[patchID + 1];

	float h0 = 2.0 * u * u * u - 3.0 * u * u + 1.0;
	float h1 = -2.0 * u * u * u + 3.0 * u * u;
	float h2 = u * u * u - 2.0 * u * u + u;
	float h3 = u * u * u - u * u;

	vec4 pos =
		h0 * gl_in[0].gl_Position +
		h1 * gl_in[1].gl_Position +
		h2 * tan0 +
		h3 * tan1;

	gl_Position = matProj * vec4(pos.xyz, 1.0);
}
