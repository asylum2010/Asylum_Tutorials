
uniform sampler2D renderedscene : register(s0);

uniform float2 pixelSize;
uniform float exposure = 1.0f;

static const float A = 0.22;
static const float B = 0.30;
static const float C = 0.10;
static const float D = 0.20;
static const float E = 0.01;
static const float F = 0.30;
static const float W = 11.2;

float3 FilmicTonemap(float3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void vs_main(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_main(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float4 scene = tex2D(renderedscene, tex);

	float3 lincolor = FilmicTonemap(scene.rgb * exposure);
	float3 invlinwhite = 1.0f / FilmicTonemap(float3(W, W, W));

	color.rgb = lincolor * invlinwhite;
	color.a = scene.a;
}

technique tonemap
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
