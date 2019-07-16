
#include "pbr_common.fx"

#define NUM_SAMPLES	2048

static const float4 vertices[4] =
{
	{ -1, -1, 0.5f, 1 },
	{ -1, 1, 0.5f, 1 },
	{ 1, -1, 0.5f, 1 },
	{ 1, 1, 0.5f, 1 },
};

void vs_main(
	in	uint vid	: SV_VertexID,
	out	float4 opos	: SV_Position)
{
	opos = vertices[vid];
}

float2 IntegrateBRDF(float roughness, float ndotv)
{
	float3 n = { 0.0f, 0.0f, 1.0f };
	float3 v = { sqrt(1.0f - ndotv * ndotv), 0.0f, ndotv };
	float3 h, l;
	float2 xi;
	float2 result = 0;

	for (uint i = 0; i < NUM_SAMPLES; ++i) {
		xi = Hammersley(i, NUM_SAMPLES);
		h = GGXSample(xi, roughness);

		l = 2.0f * dot(v, h) * h - v;

		float ndotl = saturate(l.z);
		float ndoth = saturate(h.z);
		float vdoth = saturate(dot(v, h));

		if (ndotl > 0) {
			float Fc = pow(1 - vdoth, 5.0f);

			// PDF = (D(h) * dot(n, h)) / (4 * dot(v, h))
			float G = G_Smith_Lightprobe(ndotl, ndotv, roughness);
			float G_mul_pdf = saturate((G * vdoth) / (ndotv * ndoth));

			result.x += (1 - Fc) * G_mul_pdf;
			result.y += Fc * G_mul_pdf;
		}
	}

	result.x /= (float)NUM_SAMPLES;
	result.y /= (float)NUM_SAMPLES;

	return result;
}

void ps_main(
	in	float4 spos		: SV_Position,
	out float4 color	: SV_Target)
{
	float2 ndc = spos.xy / 256.0f;

	color.rg = IntegrateBRDF(ndc.y, ndc.x);
	color.ba = float2(0, 1);
}

technique10 integratebrdf
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
