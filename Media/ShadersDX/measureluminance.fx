
uniform sampler2D prevtarget : register(s0);

uniform float2 pixelSize;
uniform float2 texelSize;
uniform int prevLevel;

static const float3 LuminanceVector = { 0.2125, 0.7154, 0.0721 };

void vs_main(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
	pos.xy -= pixelSize.xy;
}

void ps_avgluminital(
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	float logsum = 0;

	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(-0.5f, -0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(-0.5f, 0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(-0.5f, 1.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(0.5f, -0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(0.5f, 0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(0.5f, 1.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(1.5f, -0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(1.5f, 0.5f) * texelSize).rgb, LuminanceVector));
	logsum += log(0.0001f + dot(tex2D(prevtarget, tex + float2(1.5f, 1.5f) * texelSize).rgb, LuminanceVector));

	logsum /= 9.0f;

	color = float4(logsum, 0, 0, 0);
}

void ps_avglumiterative(
	uniform bool finalpass,
	in	float2 tex : TEXCOORD0,
	out	float4 color : COLOR0)
{
	float sum = 0;

	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-1.5f, -1.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-1.5f, -0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-1.5f, 0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-1.5f, 1.5f), 0, prevLevel)).r;

	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-0.5f, -1.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-0.5f, -0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-0.5f, 0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(-0.5f, 1.5f), 0, prevLevel)).r;

	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(0.5f, -1.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(0.5f, -0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(0.5f, 0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(0.5f, 1.5f), 0, prevLevel)).r;

	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(1.5f, -1.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(1.5f, -0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(1.5f, 0.5f), 0, prevLevel)).r;
	sum += tex2Dlod(prevtarget, float4(tex + texelSize * float2(1.5f, 1.5f), 0, prevLevel)).r;

	sum *= 0.0625f;

	if (finalpass)
		color = float4(exp(sum), 0, 0, 0);
	else
		color = float4(sum, 0, 0, 0);
}

technique avgluminital
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_avgluminital();
	}
}

technique avglumiterative
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_avglumiterative(false);
	}
}

technique avglumfinal
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_avglumiterative(true);
	}
}
