
uniform sampler2D renderedscene : register(s0);

uniform float2 pixelSize;
uniform float2 texelSize;

void vs_main(
	in out	float4 pos : POSITION,
	in out	float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_boxblur3x3(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	color = 0;

	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, -0.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, 0.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, 1.5f));

	color /= 9.0f;
}

void ps_boxblur5x5(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	color = 0;

	color += tex2D(renderedscene, tex + texelSize * float2(-1.5f, -1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, -1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, -1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, -1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(2.5f, -1.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-1.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, -0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(2.5f, -0.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-1.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, 0.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(2.5f, 0.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-1.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, 1.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(2.5f, 1.5f));

	color += tex2D(renderedscene, tex + texelSize * float2(-1.5f, 2.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(-0.5f, 2.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(0.5f, 2.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(1.5f, 2.5f));
	color += tex2D(renderedscene, tex + texelSize * float2(2.5f, 2.5f));

	color /= 25.0f;
}

technique boxblur3x3
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_boxblur3x3();
	}
}

technique boxblur5x5
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_boxblur5x5();
	}
}
