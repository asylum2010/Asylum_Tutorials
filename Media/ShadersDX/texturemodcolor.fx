
uniform sampler2D baseColor : register(s0);

uniform matrix matWorld;
uniform matrix matViewProj;

uniform float2 uv = { 1, 1 };

void vs_main(
	in out float4 pos : POSITION,
	in out float4 color : COLOR0,
	in out float2 tex : TEXCOORD0)
{
	pos = mul(mul(float4(pos.xyz, 1), matWorld), matViewProj);
	tex *= uv;
}

void ps_main(
	in float2 tex : TEXCOORD0,
	in float4 color : COLOR0,
	out float4 color0 : COLOR0)
{
	float4 base = tex2D(baseColor, tex);
	color0 = base * color;
}

technique texturemodcolor
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
