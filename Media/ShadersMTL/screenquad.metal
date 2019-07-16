
#include "common.h"

INSTANTIATE_SCREENQUAD(screenquad);

fragment half4 ps_screenquad(
	texture2d<half> tex0 [[texture(0)]],
	ScreenVertex in [[stage_in]])
{
	constexpr sampler sampler0(coord::normalized, address::clamp_to_edge, filter::linear);
	
	return tex0.sample(sampler0, in.tex);
}
