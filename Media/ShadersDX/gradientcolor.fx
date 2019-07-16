
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 outerColor = { 1, 0, 1, 1 };
uniform float4 innerColor = { 1, 1, 1, 1 };

void vs_main(
	in out float4 pos : POSITION,
	in out float2 grad : TEXCOORD0)
{
	// NOTE: don't change this
	pos = mul(mul(pos, matWorld), matViewProj);
}

void ps_main(
	in float2 grad : TEXCOORD0,
	out float4 color : COLOR0)
{
	const float ColorFallOffExponent = 2.0f;

	float brightness = 1.0f - saturate(length(grad));
	float color_shift = saturate(pow(brightness, ColorFallOffExponent));
	
	color = brightness * lerp(outerColor, innerColor, color_shift);
}

technique gradientcolor
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
