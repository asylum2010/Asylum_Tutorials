
uniform samplerCUBE environment		: register(s0);
uniform sampler2D gbufferNormals	: register(s1);
uniform sampler2D gbufferDepth		: register(s2);

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;
uniform matrix matViewProjInv;

uniform float4 baseColor = { 0, 0, 0, 1 };
uniform float4 eyePos;
uniform float2 indicesOfRefraction = { 1.000293f, 1.53f };

float3 GetWorldPosition(float3 wpos)
{
	float4 cpos = mul(float4(wpos, 1), matViewProj);
	float2 spos = cpos.xy / cpos.w;
	float2 tpos = spos * float2(0.5f, -0.5f) + 0.5f;

	// NOTE: can't use tex2D in loops and branches
	float d = tex2Dlod(gbufferDepth, float4(tpos, 0, 0)).r;

	if (d == 0)
		return 0;

	cpos = float4(spos, d, 1);
	cpos = mul(cpos, matViewProjInv);

	return (cpos.xyz / cpos.w);
}

void NewtonRhapson2D(out float3 hitpos, out float3 hitnorm, in float3 start, in float3 dir)
{
	float d = 0.01f; // 1 cm
	float3 r = normalize(dir);

	hitpos = GetWorldPosition(start + d * r);
	d = length(hitpos - start);

	hitpos = GetWorldPosition(start + d * r);
	d = length(hitpos - start);

	hitpos = GetWorldPosition(start + d * r);
	d = length(hitpos - start);

	hitpos = GetWorldPosition(start + d * r);

	float4 cpos = mul(float4(hitpos, 1), matViewProj);
	float2 spos = cpos.xy / cpos.w;
	float2 tpos = spos * float2(0.5f, -0.5f) + 0.5f;

	hitnorm = tex2Dlod(gbufferNormals, float4(tpos, 0, 0)).xyz * 2.0f - 1.0f;
}

void vs_gbuffer(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float3 wnorm	: TEXCOORD0,
	out		float4 cpos		: TEXCOORD1)
{
	cpos = mul(mul(float4(pos.xyz, 1), matWorld), matViewProj);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	pos = cpos;
}

void ps_gbuffer(
	in float3 wnorm		: TEXCOORD0,
	in float4 cpos		: TEXCOORD1,
	out float4 color0	: COLOR0,
	out float4 color1	: COLOR1)
{
	float3 n = normalize(wnorm);

	color0.rgb = n * 0.5f + 0.5f;
	color0.a = 1;

	color1 = float4(cpos.z / cpos.w, 0, 0, 0);
}

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float3 wnorm	: TEXCOORD0,
	out		float4 wpos		: TEXCOORD1)
{
	wpos = mul(float4(pos.xyz, 1), matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	pos = mul(wpos, matViewProj);
}

void ps_reflection(
	in	float3 wnorm	: TEXCOORD0,
	in	float4 wpos		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float3 n = normalize(wnorm);
	float3 v = normalize(wpos.xyz - eyePos.xyz);
	float3 r = reflect(v, n);

	float ndotv = saturate(-dot(n, v));
	float n1 = indicesOfRefraction.x;
	float n2 = indicesOfRefraction.y;

	float3 F0 = (n1 - n2) / (n1 + n2);
	F0 *= F0;

	float4 refl = texCUBE(environment, r);
	float3 F = F0 + (1.0f - F0) * pow(1.0f - ndotv + 1e-5f, 5.0f);

	color.rgb = baseColor.rgb * (1 - F) + refl.rgb * F;
	color.a = 1;
}

void ps_refraction(
	in	float3 wnorm	: TEXCOORD0,
	in	float4 wpos		: TEXCOORD1,
	out	float4 color	: COLOR0,
	uniform bool twosided)
{
	float3 n = normalize(wnorm);
	float3 v = normalize(wpos.xyz - eyePos.xyz);
	float3 r = reflect(v, n);

	float ndotv = saturate(-dot(n, v));
	float n1 = indicesOfRefraction.x;
	float n2 = indicesOfRefraction.y;

	float3 F0 = (n1 - n2) / (n1 + n2);
	F0 *= F0;

	// reflection
	float4 refl = texCUBE(environment, r);
	float3 F = F0 + (1.0f - F0) * pow(1.0f - ndotv + 1e-5f, 5.0f);
	float3 radiance = F * refl.rgb;

	// first refraction
	float3 t1 = refract(v, n, n1 / n2);

	if (twosided) {
		if (dot(t1, t1) > 0) {
			// ray entered the object
			float3 hitpos;

			// find hit point
			NewtonRhapson2D(hitpos, n, wpos.xyz, t1);

			if (dot(hitpos, hitpos) > 0) {
				// second refraction
				float3 t2 = refract(t1, -n, n2 / n1);

				if (dot(t2, t2) > 0) {
					// hit sky
					radiance += baseColor * texCUBE(environment, t2) * (1 - F);
				} else {
					// total internal reflection; this is where you would need front faces
					// for now let's do something that doesn't look stupid
					radiance += baseColor * texCUBE(environment, v) * (1 - F);
				}
			} else {
				// missed, just let the ray through
				radiance += baseColor * texCUBE(environment, t1) * (1 - F);
			}
		}

		color.rgb = radiance;
		color.a = 1;
	} else {
		radiance += baseColor * texCUBE(environment, t1) * (1 - F);

		color.rgb = radiance;
		color.a = 1;
	}
}

technique gbuffer
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_gbuffer();
		pixelshader = compile ps_3_0 ps_gbuffer();
	}
}

technique reflection
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_reflection();
	}
}

technique singlerefraction
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_refraction(false);
	}
}

technique doublerefraction
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_refraction(true);
	}
}
