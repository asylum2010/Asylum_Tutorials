
#define MAX_MATRICES 26

uniform sampler2D baseColor : register(s0);

uniform float4x3 matBones[MAX_MATRICES];
uniform float4x4 matWorld;		// when not skinned
uniform float4x4 matViewProj;

uniform int numBones;

void vs_skinned(
	in out	float4 pos		: POSITION,
	in		float4 weights	: BLENDWEIGHT,
	in		float4 indices	: BLENDINDICES,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	uniform int bones)
{
	float3 bpos = 0.0f;
	float3 bnorm = 0.0f;

	int4 ind = D3DCOLORtoUBYTE4(indices);
	int blendindices[4] = (int[4])ind;

	float blendweights[4] = (float[4])weights;
	float last = 0;

	for (int i = 0; i < bones - 1; ++i) {
		last += blendweights[i];

		bpos += mul(pos, matBones[blendindices[i]]) * blendweights[i];
		bnorm += mul(norm, matBones[blendindices[i]]) * blendweights[i];
	}

	// must always sum up to 1
	last = 1 - last;

	bpos += mul(pos, matBones[blendindices[bones - 1]]) * last;
	bnorm += mul(norm, matBones[blendindices[bones - 1]]) * last;

	pos = mul(float4(bpos.xyz, 1), matViewProj);
}

void vs_regular(
	in out	float4 pos	: POSITION,
	in out	float2 tex	: TEXCOORD0)
{
	pos = mul(mul(pos, matWorld), matViewProj);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	color = tex2D(baseColor, tex);
}

vertexshader vsarray[4] = {
	compile vs_3_0 vs_skinned(1),
	compile vs_3_0 vs_skinned(2),
	compile vs_3_0 vs_skinned(3),
	compile vs_3_0 vs_skinned(4)
};

technique skinned
{
	pass p0
	{
		vertexshader = vsarray[numBones];
		pixelshader = compile ps_3_0 ps_main();
	}
}

technique regular
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_regular();
		pixelshader = compile ps_3_0 ps_main();
	}
}
