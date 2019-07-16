
uniform Texture2D image;

SamplerState linearSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

static const float4 vertices[4] =
{
	{ -1, -1, 0.5f, 1 },
	{ -1, 1, 0.5f, 1 },
	{ 1, -1, 0.5f, 1 },
	{ 1, 1, 0.5f, 1 },
};

static const float2 texcoords[4] =
{
	{ 0, 1 },
	{ 0, 0 },
	{ 1, 1 },
	{ 1, 0 },
};

void vs_main(
	in	uint vid	: SV_VertexID,
	out float2 tex	: TEXCOORD0,
	out	float4 opos	: SV_Position)
{
	opos = vertices[vid];
	tex = texcoords[vid];
}

void ps_main(
	in	float2 tex : TEXCOORD0,
	out	float4 color : SV_Target)
{
	color = image.Sample(linearSampler, tex);
}

technique10 screenquad
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
