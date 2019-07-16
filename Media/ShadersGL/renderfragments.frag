
#version 430

struct ListHead
{
	uvec4 StartAndCount;
};

struct ListNode
{
	uvec4 ColorDepthNext;
};

layout(std140, binding = 0) readonly buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(std140, binding = 1) readonly buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

uniform int screenWidth;

out vec4 my_FragColor0;

void main()
{
	ivec2	fragID	= ivec2(gl_FragCoord.xy);
	int		index	= fragID.y * screenWidth + fragID.x;
	uint	nodeID	= headbuffer.data[index].StartAndCount.x;
	uint	count	= headbuffer.data[index].StartAndCount.y;
	vec4	color	= vec4(0.0, 0.0, 0.0, 1.0);
	vec4	fragment;

	for (int i = 0; i < count; ++i) {
		fragment = unpackUnorm4x8(nodebuffer.data[nodeID].ColorDepthNext.x);

		color.rgb = mix(color.rgb, fragment.rgb, fragment.a);
		color.a *= (1.0 - fragment.a);

		nodeID = nodebuffer.data[nodeID].ColorDepthNext.z;
	}

	my_FragColor0 = color;
}
