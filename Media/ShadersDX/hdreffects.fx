
uniform sampler2D renderedscene		: register(s0);
uniform sampler2D previoustarget	: register(s0);
uniform sampler2D averageluminance	: register(s1);

uniform sampler2D blurtarget0		: register(s0);
uniform sampler2D blurtarget1		: register(s1);
uniform sampler2D blurtarget2		: register(s2);
uniform sampler2D blurtarget3		: register(s3);
uniform sampler2D blurtarget4		: register(s4);

uniform float2 pixelSize;
uniform float2 texelSize;
uniform float exposure;

uniform int blurDirection;
uniform int starDirection;
uniform int starPass;

static const float2 blurOffsets[9] =
{
	{ -4, 0 },
	{ -3, 0 },
	{ -2, 0 }, 
	{ -1, 0 },
	{ 0,  0 },
	{ 1,  0 },
	{ 2,  0 },
	{ 3,  0 }, 
	{ 4,  0 }
};

static const float blurWeights[9] =
{
	0.013437f,
	0.047370f,
	0.116512f,
	0.199935f,
	0.239365f,
	0.199935f,
	0.116512f,
	0.047370f,
	0.013437f
};

static const float2 starOffsets[4] =
{
	{ -1, -1 },
	{  1, -1 },
	{ -1,  1 },
	{  1,  1 }
};

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

void ps_brightpass(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	// NOTE: HDR effects are dependent on exposure
	color = tex2D(renderedscene, tex);

	color = color * (exposure * 0.002f) - 0.002f;
	color = min(max(color, 0), 16384.0f);

	color.a = 1;
}

void ps_downsample(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	color = 0;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			color += tex2D(previoustarget, tex + texelSize * float2(i - 1.5f, j - 1.5f));
		}
	}

	color *= 0.0625f;
	color.a = 1;
}

void ps_blur(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	color = 0;

	if (blurDirection == 0) {
		for (int i = 0; i < 9; ++i)
			color += tex2D(previoustarget, tex + texelSize * blurOffsets[i].xy) * blurWeights[i];
	} else {
		for (int i = 0; i < 9; ++i)
			color += tex2D(previoustarget, tex + texelSize * blurOffsets[i].yx) * blurWeights[i];
	}

	color.a = 1;
}

void ps_blurcombine(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float4 c0 = tex2D(blurtarget0, tex) * 5.0f;
	float4 c1 = tex2D(blurtarget1, tex) * 7.0f;
	float4 c2 = tex2D(blurtarget2, tex) * 6.5f;
	float4 c3 = tex2D(blurtarget3, tex) * 3.5f;
	float4 c4 = tex2D(blurtarget4, tex) * 1.75f;

	color = c0 + c1 + c2 + c3 + c4;
	color.a = 1;
}

void ps_star(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	color = 0;

	float b = pow(4.0f, starPass);
	float a = 0.9f;

	float2 off = starOffsets[starDirection];
	float2 offtex;

	for (int i = 0; i < 4; ++i) {
		offtex = tex + b * i * texelSize * off;
		color += tex2D(renderedscene, offtex) * pow(a, b * i);
	}

	color.a = 1;
}

void ps_starcombine(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float4 c0 = tex2D(blurtarget0, tex);
	float4 c1 = tex2D(blurtarget1, tex);
	float4 c2 = tex2D(blurtarget2, tex);
	float4 c3 = tex2D(blurtarget3, tex);

	color = c0 + c1 + c2 + c3;
	color.a = 1;
}

void ps_lensflare(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float2 a = (tex - 0.5f) * -1.724f;
	float2 b = (tex - 0.5f) * -2.845f;
	float2 c = (tex - 0.5f) * -1.0f;
	float2 d = (tex - 0.5f) * -1.957f;
	float2 e = (tex - 0.5f) * -2.147f;
	float2 f = (tex - 0.5f) * -4.0f;
	float2 g = (tex - 0.5f) * -1.794f;
	
	float sa = saturate(1 - dot(a, a) * 3);
	float sb = saturate(1 - dot(b, b) * 3);
	float sc = saturate(1 - dot(c, c) * 3);
	float sd = saturate(1 - dot(d, d) * 3);
	float se = saturate(1 - dot(e, e) * 3);
	float sf = saturate(1 - dot(f, f) * 3);
	float sg = saturate(1 - dot(g, g) * 3);
	
	a += 0.5f;
	b += 0.5f;
	c += 0.5f;
	d += 0.5f;
	e += 0.5f;
	f += 0.5f;
	g += 0.5f;

	color = tex2D(blurtarget0, a) * sa * float4(0.250f, 0.175f, 0.125f, 1);
	color += tex2D(blurtarget1, b) * sb * float4(0.131f, 0.187f, 0.131f, 1);
	color += tex2D(blurtarget0, c) * sc * float4(0.103f, 0.103f, 0.103f, 1);
	color += tex2D(blurtarget1, d) * sd * float4(0.2f, 0.2f, 0.250f, 1);
	color += tex2D(blurtarget0, e) * se * float4(0.101f, 0.050f, 0.050f, 1);
	color += tex2D(blurtarget1, f) * sf * float4(0.102f, 0.102f, 0.102f, 1);
	color += tex2D(blurtarget2, g) * sg * float4(0.248f, 0.248f, 0.248f, 1);
	
	color.a = 1;
}

void ps_lensflare_expand(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float2 a = (tex - 0.5f);
	float2 b = (tex - 0.5f) * 5.0f;
	float2 c = (tex - 0.5f) * -4.0f;
	float2 d = (tex - 0.5f) * -1.33f;
	float2 e = (tex - 0.5f) * 0.5f;
	
	float sa = saturate(1 - dot(a, a) * 3);
	float sb = saturate(1 - dot(b, b) * 3);
	float sc = saturate(1 - dot(c, c) * 3);
	float sd = saturate(1 - dot(d, d) * 3);
	float se = saturate(1 - dot(e, e) * 3);
	
	a += 0.5f;
	b += 0.5f;
	c += 0.5f;
	d += 0.5f;
	e += 0.5f;
	
	color = tex2D(blurtarget0, a) * sa * float4(0.413f, 0.413f, 2.063f, 1);
	color += tex2D(blurtarget0, b) * sb * float4(1.611f, 0.644f, 0.644f, 1);
	color += tex2D(blurtarget0, c) * sc * float4(1.000f, 2.000f, 0.800f, 1);
	color += tex2D(blurtarget0, d) * sd * float4(0.254f, 0.254f, 0.845f, 1);
	color += tex2D(blurtarget0, e) * se * float4(0.315f, 0.701f, 0.315f, 1);
	
	color.a = 1;
}

void ps_afterimage(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR)
{
	// NOTE: afterimage is dependent on framerate
	float4 prev = tex2D(blurtarget0, tex);
	float4 curr = tex2D(blurtarget1, tex);

	color = lerp(prev, curr, 0.1f);
	color = max(color, 0);

	color.a = 1;
}

void ps_tonemap(
	in float2 tex : TEXCOORD0,
	out float4 color : COLOR0)
{
	float4 scene	= tex2D(renderedscene, tex);
	float4 bloom	= tex2D(blurtarget1, tex);
	float4 star		= tex2D(blurtarget2, tex);
	float4 ghost	= tex2D(blurtarget3, tex);
	float4 afterimg	= tex2D(blurtarget4, tex);

	float3 lincolor = FilmicTonemap(scene.rgb * exposure);
	float3 invlinwhite = 1.0f / FilmicTonemap(float3(W, W, W));

	color.rgb = lincolor * invlinwhite;
	color.rgb += bloom.rgb + star.rgb + ghost.rgb + afterimg.rgb;

	tex -= 0.5f;
	float vignette = 1 - dot(tex, tex);

	color.rgb *= vignette * vignette * vignette;
	color.a = 1.0f;
}

technique brightpass
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_brightpass();
	}
}

technique downsample
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_downsample();
	}
}

technique blur
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_blur();
	}
}

technique blurcombine
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_blurcombine();
	}
}

technique star
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_star();
	}
}

technique starcombine
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_starcombine();
	}
}

technique lensflare
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_lensflare();
	}
}

technique lensflare_expand
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_lensflare_expand();
	}
}

technique afterimage
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_afterimage();
	}
}

technique tonemap
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_tonemap();
	}
}
