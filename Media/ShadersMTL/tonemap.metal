
#include "common.h"

INSTANTIATE_SCREENQUAD(tonemap);

constant float A = 0.22;
constant float B = 0.30;
constant float C = 0.10;
constant float D = 0.20;
constant float E = 0.01;
constant float F = 0.30;
constant float W = 11.2;

float3 FilmicTonemap(float3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

fragment float4 ps_tonemap(
	texture2d<float> tex0 [[texture(0)]],
	ScreenVertex in [[stage_in]])
{
	uint2 loc = uint2(in.pos.xy);
	float exposure = 1.0f;

	float4 base = tex0.read(loc, 0);
	float3 lincolor = FilmicTonemap(base.rgb * exposure);
	float3 invlinwhite = 1.0f / FilmicTonemap(W);

	base.rgb = lincolor * invlinwhite;
	
	return base;
}
