
uniform sampler intensityRamp	: register(s0);

uniform sampler normalsAndDepth	: register(s0);

uniform sampler renderedScene	: register(s0);
uniform sampler detectedEdges	: register(s1);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos		= { -5, 10, -10, 1 };
uniform float4 baseColor;
uniform float2 clipPlanes;
uniform float2 texelSize;

void vs_celshading(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float4 cpos		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2)
{
	pos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	ldir = lightPos.xyz - pos.xyz;
	cpos = mul(pos, matViewProj);

	pos = cpos;
}

void vs_screenquad(
	in out float4 pos : POSITION,
	in out float2 tex : TEXCOORD0)
{
}

void ps_celshading(
	in	float4 cpos		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	out float4 color	: COLOR0,
	out float4 normd	: COLOR1)
{
	float3 l = normalize(ldir);
	float3 n = normalize(wnorm);

	float ndotl = saturate(dot(n, l));
	ndotl = tex1D(intensityRamp, ndotl);

	float z = cpos.w;
	float d = (z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

	color.rgb = baseColor.rgb * ndotl;
	color.a = baseColor.a;

	normd.rgb = n * 0.5f + 0.5f;
	normd.w = d;
}

void ps_edgedetect(
	in	float2 tex		: TEXCOORD0,
	out float4 color	: COLOR0)
{
	float4 n1 = tex2D(normalsAndDepth, tex + float2(-1, -1) * texelSize);
	float4 n2 = tex2D(normalsAndDepth, tex + float2(0, -1) * texelSize);
	float4 n3 = tex2D(normalsAndDepth, tex + float2(1, -1) * texelSize);
	float4 n4 = tex2D(normalsAndDepth, tex + float2(-1, 1) * texelSize);
	float4 n5 = tex2D(normalsAndDepth, tex + float2(0, 1) * texelSize);
	float4 n6 = tex2D(normalsAndDepth, tex + float2(1, 1) * texelSize);
	float4 n7 = tex2D(normalsAndDepth, tex + float2(-1, 0) * texelSize);
	float4 n8 = tex2D(normalsAndDepth, tex + float2(1, 0) * texelSize);

	n1.rgb = n1.rgb * 2.0f - 1.0f;
	n2.rgb = n2.rgb * 2.0f - 1.0f;
	n3.rgb = n3.rgb * 2.0f - 1.0f;
	n4.rgb = n4.rgb * 2.0f - 1.0f;
	n5.rgb = n5.rgb * 2.0f - 1.0f;
	n6.rgb = n6.rgb * 2.0f - 1.0f;
	n7.rgb = n7.rgb * 2.0f - 1.0f;
	n8.rgb = n8.rgb * 2.0f - 1.0f;

	// Sobel operator (normal gradient)
	float3 ngx = -n1.rgb - 2 * n2.rgb - n3.rgb + n4.rgb + 2 * n5.rgb + n6.rgb;
	float3 ngy = -n1.rgb - 2 * n7.rgb - n4.rgb + n3.rgb + 2 * n8.rgb + n6.rgb;
	float3 ng = sqrt(ngx * ngx + ngy * ngy);

	// depth gradient
	ngx.x = -n1.a - 2 * n2.a - n3.a + n4.a + 2 * n5.a + n6.a;
	ngx.y = -n1.a - 2 * n7.a - n4.a + n3.a + 2 * n8.a + n6.a;

	float dg = sqrt(ngx.x * ngx.x + ngx.y * ngx.y);

	// detect edges (this part could be improved)
	float contribution1 = saturate(dot(ng, 1.0f) - 3.5f);
	float contribution2 = saturate(dg * 10 - 0.4f);
	float supressed = saturate(contribution1 + contribution2);

	color = supressed.xxxx;
}

void ps_compose(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float contour = tex2D(detectedEdges, tex).r;
	color = tex2D(renderedScene, tex) * (1.0f - contour);
}

technique celshading
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_celshading();
		pixelshader = compile ps_3_0 ps_celshading();
	}
}

technique edgedetect
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_screenquad();
		pixelshader = compile ps_3_0 ps_edgedetect();
	}
}

technique compose
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_screenquad();
		pixelshader = compile ps_3_0 ps_compose();
	}
}
