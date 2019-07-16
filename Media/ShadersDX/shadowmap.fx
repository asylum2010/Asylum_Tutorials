
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 lightPos;
uniform float2 clipPlanes;
uniform bool isPerspective;	// can't decide from LH matrices

void vs_variance(
	in out	float4 pos	: POSITION,
	out		float2 zw	: TEXCOORD0,
	out		float4 wpos	: TEXCOORD1)
{
	wpos = mul(float4(pos.xyz, 1), matWorld);
	pos = mul(wpos, matViewProj);

	zw.xy = pos.zw;
}

void ps_variance(
	in	float2 zw		: TEXCOORD0,
	in	float4 wpos		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float d = zw.x;

	if (isPerspective) {
		float z = length(lightPos.xyz - wpos.xyz);
		d = (z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	}

	color = float4(d, d * d, 0, 1);
}

technique variance
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_variance();
		pixelshader = compile ps_3_0 ps_variance();
	}
}
