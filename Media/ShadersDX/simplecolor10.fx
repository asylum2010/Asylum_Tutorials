
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 matColor = { 1, 0, 0, 1 };

void vs_main(
	in	float3 pos : POSITION,
	out	float4 opos : SV_Position)
{
	opos = mul(mul(float4(pos, 1), matWorld), matViewProj);
}

void ps_main(
	out float4 color : SV_Target)
{
	color = matColor;
}

technique10 simplecolor
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
