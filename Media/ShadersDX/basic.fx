
uniform matrix matWVP;

void vs_main(
	in out float4 pos : POSITION)
{
	// be careful how you multiply
	pos = mul(float4(pos.xyz, 1), matWVP);
}

void ps_main(
	out float4 color : COLOR0)
{
	// green
	color = float4(0, 1, 0, 1);
}

technique basic
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
