
uniform sampler2D sampler0 : register(s0);

uniform float2 pixelSize;
uniform float2 texelSize;
uniform float2 center;
uniform float exposure;

static const float Density	= 0.8f;	// limits blur distance
static const float Decay	= 1.0f;	// smaller value means faster decay
static const float Weight	= 1.0f;	// weighs samples

void vs_main(
	in out	float4 pos : POSITION,
	in out	float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_radialblur(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	const int NumSamples = 125;

	float2 delta = tex - center;
	float samp;
	float illum = 1.0f;

	color = tex2D(sampler0, tex).a;

	// step size in texture space
	delta *= Density / NumSamples;

	for (int i = 0; i < NumSamples; ++i) {
		tex -= delta;

		samp = tex2D(sampler0, tex).a;
		samp *= (illum * Weight);

		color += samp;
		illum *= Decay;
	}

	color /= NumSamples;
	color *= exposure;
}

technique radialblur
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_radialblur();
	}
}
