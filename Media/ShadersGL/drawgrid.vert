
#version 430

subroutine vec4 CalcVertexFunc();

in vec3 my_Position;
layout(location = 0) uniform mat4 matProj;
layout(location = 1) uniform int numControlVertices;

// NOTE: this is patch data!!! (0, 1, 1, 2, 2, 3, 3, ..., n)
layout(std140, binding = 0) readonly buffer ControlVertexData {
	vec4 data[];
} controlVertices;

subroutine uniform CalcVertexFunc vertexFunc;

layout(index = 0) subroutine(CalcVertexFunc) vec4 CalcPointVertex()
{
	return matProj * vec4(my_Position, 1.0);
}

layout(index = 1) subroutine(CalcVertexFunc) vec4 CalcTangentVertex()
{
	int index = min(gl_VertexID * 2, numControlVertices - 1);
	vec3 pos = controlVertices.data[index].xyz;

	return matProj * vec4(pos + my_Position, 1.0);
}

void main()
{
	gl_Position = vertexFunc();
}
