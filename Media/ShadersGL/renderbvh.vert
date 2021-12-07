
#version 450

#include "raytrace_structs.head"

in vec3 my_Position;

uniform mat4 matViewProj;

layout (std430, binding = 0) readonly buffer BVHierarchy {
	BVHNode data[];
} hierarchy;

out vec4 color;

void main()
{
	BVHNode node = hierarchy.data[gl_InstanceID];

	vec3 size = (node.Max - node.Min);
	vec3 center = (node.Min + node.Max) * 0.5;
	vec3 pos = my_Position * size + center;

	color = (((node.LeftOrCount & 0x80000000) == 0x80000000) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0));

	gl_Position = matViewProj * vec4(pos, 1.0);
}
