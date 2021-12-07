
Texture2D baseColor : register(t0);
SamplerState linearSampler : register(s0);

cbuffer UniformData : register(b1) {
	float4x4 matWorld;
	float4x4 matWorldInv;
	float4x4 matViewProj;

	float4 lightPos;
	float4 lightColor;
	float4 lightAmbient;
	float4 eyePos;
	float2 uv;
};

struct VS_INPUT {
	float3 pos	: POSITION;
	float3 norm	: NORMAL;
	float2 tex	: TEXCOORD0;
};

struct VS_OUTPUT {
	float4 position	: SV_Position;
	float3 wnorm	: TEXCOORD0;
	float3 ldir		: TEXCOORD1;
	float3 vdir		: TEXCOORD2;
	float2 tex		: TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 wpos = mul(matWorld, float4(input.pos, 1));

	output.wnorm = mul(float4(input.norm, 0), matWorldInv).xyz;
	output.ldir = lightPos.xyz - wpos.xyz;
	output.vdir = eyePos.xyz - wpos.xyz;

	output.position = mul(matViewProj, wpos);
	output.tex = input.tex * uv;

	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
	float3 l = normalize(input.ldir);
	float3 v = normalize(input.vdir);
	float3 h = normalize(l + v);
	float3 n = normalize(input.wnorm);

	float4 base = baseColor.Sample(linearSampler, input.tex);

	float ndotl = saturate(dot(n, l));
	float ndoth = saturate(dot(n, h));

	float3 f_diffuse = base.rgb;
	float f_specular = pow(ndoth, 80);
	float4 color;

	color.rgb = base.rgb * lightAmbient.rgb + (f_diffuse + f_specular) * lightColor.rgb * ndotl;
	color.a = base.a;

	return color;
}
