
#ifndef _COMMON_H_
#define _COMMON_H_

#include <metal_stdlib>

using namespace metal;

struct ScreenVertex
{
	float4 pos [[ position ]];
	float2 tex;
};

#define INSTANTIATE_SCREENQUAD(name) \
	vertex ScreenVertex vs_##name( \
		uint vid [[vertex_id]]) \
	{ \
		const float4 positions[4] = { \
			float4(-1, 1, 0.5, 1), \
			float4(1, 1, 0.5, 1), \
			float4(-1, -1, 0.5, 1), \
			float4(1, -1, 0.5, 1) \
		}; \
		\
		const float2 texcoords[4] = { \
			float2(0, 0), \
			float2(1, 0), \
			float2(0, 1), \
			float2(1, 1) \
		}; \
		\
		ScreenVertex out; \
		\
		out.pos = positions[vid]; \
		out.tex = texcoords[vid]; \
		 \
		return out; \
	}
// END

#endif

