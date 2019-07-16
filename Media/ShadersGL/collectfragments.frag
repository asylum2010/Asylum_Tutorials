
#version 430

#define MAX_LAYERS	4

struct ListHead
{
	uvec4 StartAndCount;
};

struct ListNode
{
	uvec4 ColorDepthNext;
};

layout(std140, binding = 0) buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(std140, binding = 1) buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

layout(binding = 0) uniform atomic_uint nextInsertionPoint;

uniform vec4 matColor;
uniform int screenWidth;

in vec4 vpos;

void main()
{
	vec4	color	= matColor;
	uvec4	nodes[MAX_LAYERS + 1];
	uvec4	tmp;
	ivec2	fragID	= ivec2(gl_FragCoord.xy);
	int		index	= fragID.y * screenWidth + fragID.x;

	uint	depth	= floatBitsToUint(vpos.z);
	uint	head	= headbuffer.data[index].StartAndCount.x;
	uint	count	= headbuffer.data[index].StartAndCount.y;
	uint	next	= head;
	uint	node;

	nodes[0].x = packUnorm4x8(color);
	nodes[0].y = depth;
	nodes[0].z = 0xFFFFFFFF;
	nodes[0].w = 0;

	// load to private memory
	for (uint i = 1; i <= count; ++i) {
		nodes[i] = nodebuffer.data[next].ColorDepthNext;
		next = nodes[i].z;

		float test = uintBitsToFloat(nodes[i].y);

		if (abs(test - vpos.z) < 0.02) // 2 cm
			return;
	}

	// sort
	for (uint i = 1; i < count + 1; ++i) {
		uint j = i;

		while (j > 0 && nodes[j - 1].y < nodes[j].y) {
			tmp = nodes[j - 1];
			nodes[j - 1] = nodes[j];
			nodes[j] = tmp;

			--j;
		}
	}

	// write back
	next = head;

	for (uint i = 1; i <= count; ++i) {
		nodebuffer.data[next].ColorDepthNext.xy = nodes[i].xy;
		next = nodebuffer.data[next].ColorDepthNext.z;
	}

	// insert new as head
	if (count < MAX_LAYERS) {
		node = atomicCounterIncrement(nextInsertionPoint);
		nodes[0].z = head;

		nodebuffer.data[node].ColorDepthNext = nodes[0];
		headbuffer.data[index].StartAndCount.x = node;
		headbuffer.data[index].StartAndCount.y = count + 1;
	}
}
