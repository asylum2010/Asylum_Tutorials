
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 matColor = { 1, 0, 0, 1 };

void vs_main(
	in out float4 pos : POSITION)
{
	// NOTE: don't change this
	pos = mul(mul(pos, matWorld), matViewProj);
}

void ps_main(
	out float4 color : COLOR0)
{
	color = matColor;
}

technique simplecolor
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
