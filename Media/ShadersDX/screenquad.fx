
uniform sampler2D sampler0 : register(s0);
uniform float4 rectColor = { 1, 1, 1, 1 };

void vs_main(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
}

void ps_screenquad(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	color = tex2D(sampler0, tex);
}

void ps_rect(
	out float4 color : COLOR0)
{
	color = rectColor;
}

technique screenquad
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_screenquad();
	}
}

technique rect
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_rect();
	}
}
