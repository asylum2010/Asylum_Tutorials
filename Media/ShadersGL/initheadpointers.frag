
#version 430

struct ListHead
{
	uvec4 StartAndCount;
};

layout(std140, binding = 0) writeonly buffer HeadBuffer {
	ListHead data[];
} headbuffer;

uniform int screenWidth;

void main()
{
	ivec2 fragID = ivec2(gl_FragCoord.xy);
	int index = fragID.y * screenWidth + fragID.x;

	headbuffer.data[index].StartAndCount.x = 0xFFFFFFFF;
	headbuffer.data[index].StartAndCount.y = 0;
}
