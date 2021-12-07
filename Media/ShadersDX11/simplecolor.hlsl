

cbuffer VS_GLOBALS : register(b0)
{
	matrix matProj : packoffset(c0);
};

StructuredBuffer<matrix> instanceData : register(t0);

struct VS_INPUT
{
	float3 pos : POSITION;
	float4 color : COLOR;
	uint instID : SV_InstanceID;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

VS_OUTPUT vs_simplecolor(VS_INPUT input)
{
	VS_OUTPUT output;
	matrix world = instanceData[input.instID];

	output.pos = mul(float4(input.pos, 1), world);
	output.pos = mul(output.pos, matProj);
	output.color = input.color;

	return output;
}

float4 ps_simplecolor(VS_OUTPUT input) : SV_Target
{
	return input.color;
}
