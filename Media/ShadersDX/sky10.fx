
uniform TextureCube environment;

uniform matrix matViewProj;
uniform matrix matSkyRotation;

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

void vs_main(
	in		float3 pos	: POSITION,
	out		float3 view	: TEXCOORD0,
	out		float4 opos	: SV_Position)
{
	view = mul(float4(pos, 1), matSkyRotation).xyz;
	opos = mul(float4(pos, 1), matViewProj);
}

void ps_main(
	in	float3 view		: TEXCOORD0,
	out	float4 color	: SV_Target)
{
	color = environment.Sample(linearSampler, view);
}

technique10 sky
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
