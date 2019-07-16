
uniform samplerCUBE environment : register(s0);

uniform matrix matViewProj;
uniform matrix matSkyRotation;

void vs_main(
	in out	float4 pos	: POSITION,
	out		float3 view	: TEXCOORD0)
{
	view = mul(float4(pos.xyz, 1), matSkyRotation).xyz;
	pos = mul(pos, matViewProj);
}

void ps_main(
	in	float3 view		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	color = texCUBE(environment, view);
}

technique sky
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
