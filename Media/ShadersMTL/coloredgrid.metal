
#include <metal_stdlib>

using namespace metal;

kernel void coloredgrid(
	texture2d<half, access::write> tex0 [[texture(0)]],
	uint2 loc [[thread_position_in_grid]],
	constant float4& time [[buffer(0)]])
{
	half4 color = { 0.0, 0.0, 0.0, 1.0 };
	
	if ((loc.x / 16 + loc.y / 16) % 2 == 1) {
		color.r = sin(time.x) * 0.5 + 0.5;
		color.g = cos(time.x) * 0.5 + 0.5;
		color.b = sin(time.x) * cos(time.x) * 0.5 + 0.5;
	}
	
	tex0.write(color, loc);
}
