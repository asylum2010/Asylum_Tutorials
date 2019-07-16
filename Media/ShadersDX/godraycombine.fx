
uniform sampler2D renderedScene	: register(s0);
uniform sampler2D godRays		: register(s1);

uniform float3 lightColor	= { 1, 1, 1 };
uniform float2 pixelSize;

void vs_main(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_combine(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	color = tex2D(renderedScene, tex);

	// increase contrast and dampen
	float shaft = tex2D(godRays, tex).a;
	shaft = pow(shaft, 1.8f) * 0.5f;

	// add vignette
	tex -= 0.5f;

	float vignette = 1.0f - dot(tex, tex);
	vignette *= vignette;

	color.rgb += shaft * lightColor;
	color.rgb *= vignette;
}

technique godraycombine
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_combine();
	}
}
