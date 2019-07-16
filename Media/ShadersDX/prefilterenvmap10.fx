
#include "pbr_common.fx"

#define NUM_DIFF_SAMPLES	4096
#define NUM_SPEC_SAMPLES	2048

uniform TextureCube environment;

uniform matrix matProj;
uniform matrix matCubeViews[6];
uniform float roughness;

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

struct GS_Input
{
	float4 pos	: SV_Position;
};

struct GS_Output
{
	float4 pos : SV_Position;
	float3 view : TEXCOORD0;
	uint rtindex : SV_RenderTargetArrayIndex;
};

void vs_main(
	in	float3 pos	: POSITION,
	out	GS_Input output)
{
	output.pos = float4(pos, 1);
}

[maxvertexcount(18)]
void gs_main(
	in triangle GS_Input input[3],
	in out TriangleStream<GS_Output> stream)
{
	GS_Output output;

	for (uint i = 0; i < 6; ++i) {
		output.rtindex = i;

		output.pos = mul(mul(input[0].pos, matCubeViews[i]), matProj);
		output.view = input[0].pos.xyz;

		stream.Append(output);

		output.pos = mul(mul(input[1].pos, matCubeViews[i]), matProj);
		output.view = input[1].pos.xyz;

		stream.Append(output);

		output.pos = mul(mul(input[2].pos, matCubeViews[i]), matProj);
		output.view = input[2].pos.xyz;

		stream.Append(output);
		stream.RestartStrip();
	}
}

float3 PrefilterEnvMapDiffuse(float3 n, float3 pixel)
{
	float3 color = 0;

	[loop]
	for (uint i = 0; i < NUM_DIFF_SAMPLES; ++i) {
		float2 xi = Hammersley(i, NUM_DIFF_SAMPLES);
		//float2 xi = Random(pixel, float(i));
		float3 l = TangentToWorld(CosineSample(xi), n);

		color += environment.SampleLevel(linearSampler, l, 0).rgb;
	}

	return color / NUM_DIFF_SAMPLES;
}

float3 PrefilterEnvMapSpecular(float roughness, float3 r)
{
	float3 color = 0;
	float weight = 0;

	[loop]
	for (uint i = 0; i < NUM_SPEC_SAMPLES; ++i) {
		float2 xi = Hammersley(i, NUM_SPEC_SAMPLES);
		float3 h = TangentToWorld(GGXSample(xi, roughness), r);
		float3 l = 2 * dot(r, h) * h - r;
		
		float ndotl = saturate(dot(r, l));

		if (ndotl > 0) {
			color += environment.SampleLevel(linearSampler, l, 0).rgb * ndotl;
			weight += ndotl;
		}
	}

	return color / max(weight, 1e-3f);
}

void ps_diff(
	in	GS_Output input,
	out	float4 color : SV_Target)
{
	float3 v = normalize(input.view);

	color.rgb = PrefilterEnvMapDiffuse(v, input.pos.xyz);
	color.a = 1.0f;
}

void ps_spec(
	in	GS_Output input,
	out	float4 color : SV_Target)
{
	float3 v = normalize(input.view);

	color.rgb = PrefilterEnvMapSpecular(roughness, v);
	color.a = 1.0f;
}

technique10 prefilter_diff
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(CompileShader(gs_4_0, gs_main()));
		SetPixelShader(CompileShader(ps_4_0, ps_diff()));
	}
}

technique10 prefilter_spec
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(CompileShader(gs_4_0, gs_main()));
		SetPixelShader(CompileShader(ps_4_0, ps_spec()));
	}
}
