
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 lightPos;
uniform float4 matColor	= { 0, 0, 0, 1 };

struct GS_INPUT
{
	float4 pos : SV_Position;
};

struct GS_OUTPUT
{
	float4 opos : SV_Position;
};

void ExtrudeIfSilhouette(
	in float4 v1, in float4 v2, in float4 v3,
	in out TriangleStream<GS_OUTPUT> stream)
{
	// NOTE: (v1, v3) is the edge to be extruded
	GS_OUTPUT	outvert;
	float4		extruded1;
	float4		extruded2;
	float3		a = v2.xyz - v1.xyz;
	float3		b = v3.xyz - v1.xyz;
	float4		planeeq;
	float		dist;

	// calculate plane equation of triangle
	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v1.xyz);

	// calculate distance from light
	dist = dot(planeeq, lightPos);

	if (dist < 0) {
		// faces away from light, extrude edge
		extruded1 = float4(v1.xyz - lightPos.xyz, 0);
		extruded2 = float4(v3.xyz - lightPos.xyz, 0);

		outvert.opos = mul(v3, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(v1, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(extruded2, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(extruded1, matViewProj);
		stream.Append(outvert);

		stream.RestartStrip();
	}
}

void OutputIfSilhouette(
	in float4 v1, in float4 v2, in float4 v3,
	in out LineStream<GS_OUTPUT> stream)
{
	// NOTE: (v1, v3) is the candidate edge
	GS_OUTPUT	outvert;
	float3		a = v2.xyz - v1.xyz;
	float3		b = v3.xyz - v1.xyz;
	float4		planeeq;
	float		dist;

	// calculate plane equation of triangle
	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v1.xyz);

	// calculate distance from light
	dist = dot(planeeq, lightPos);

	if (dist < 0) {
		// faces away from light, silhouette edge
		outvert.opos = mul(v1, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(v3, matViewProj);
		stream.Append(outvert);

		stream.RestartStrip();
	}
}

void vs_extrude(
	in	float3 pos		: POSITION,
	out	float4 opos		: SV_Position)
{
	opos = mul(float4(pos, 1), matWorld);
}

[maxvertexcount(18)]
void gs_extrude(
	in triangleadj GS_INPUT verts[6], 
	in out TriangleStream<GS_OUTPUT> stream)
{
	GS_OUTPUT	outvert;
	
	// NOTE: base triangle is (0, 2, 4)
	float4 v0 = verts[0].pos;
	float4 v1 = verts[1].pos;
	float4 v2 = verts[2].pos;
	float4 v3 = verts[3].pos;
	float4 v4 = verts[4].pos;
	float4 v5 = verts[5].pos;
	
	// only interested in cases when the base triangle faces the light
	float4		planeeq;
	float3		a = v2.xyz - v0.xyz;
	float3		b = v4.xyz - v0.xyz;
	float		dist;

	// calculate plane equation of triangle
	planeeq.xyz	= cross(a, b);
	planeeq.w	= -dot(planeeq.xyz, v0.xyz);

	// calculate distance from light
	dist		= dot(planeeq, lightPos);

	if (dist > 0) {
		// extrude volume sides
		ExtrudeIfSilhouette(v0, v1, v2, stream);
		ExtrudeIfSilhouette(v2, v3, v4, stream);
		ExtrudeIfSilhouette(v4, v5, v0, stream);

		// front cap
		outvert.opos = mul(v0, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(v2, matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(v4, matViewProj);
		stream.Append(outvert);

		stream.RestartStrip();

		// back cap
		outvert.opos = mul(float4(v0.xyz - lightPos.xyz, 0), matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(float4(v4.xyz - lightPos.xyz, 0), matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(float4(v2.xyz - lightPos.xyz, 0), matViewProj);
		stream.Append(outvert);
	}
}

[maxvertexcount(18)]
void gs_silhouette(
	in triangleadj GS_INPUT verts[6],
	in out LineStream<GS_OUTPUT> stream)
{
	// NOTE: base triangle is (0, 2, 4)
	float4 v0 = verts[0].pos;
	float4 v1 = verts[1].pos;
	float4 v2 = verts[2].pos;
	float4 v3 = verts[3].pos;
	float4 v4 = verts[4].pos;
	float4 v5 = verts[5].pos;

	// only interested in cases when the base triangle faces the light
	float4		planeeq;
	float3		a = v2.xyz - v0.xyz;
	float3		b = v4.xyz - v0.xyz;
	float		dist;

	// calculate plane equation of triangle
	planeeq.xyz	= cross(a, b);
	planeeq.w	= -dot(planeeq.xyz, v0.xyz);

	// calculate distance from light
	dist		= dot(planeeq, lightPos);

	if (dist > 0) {
		OutputIfSilhouette(v0, v1, v2, stream);
		OutputIfSilhouette(v2, v3, v4, stream);
		OutputIfSilhouette(v4, v5, v0, stream);
	}
}

void ps_extrude(
	out float4 color : SV_Target)
{
	color = matColor;
}

technique10 extrude
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_extrude()));
		SetGeometryShader(CompileShader(gs_4_0, gs_extrude()));
		SetPixelShader(CompileShader(ps_4_0, ps_extrude()));
	}
}

technique10 silhouette
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_extrude()));
		SetGeometryShader(CompileShader(gs_4_0, gs_silhouette()));
		SetPixelShader(CompileShader(ps_4_0, ps_extrude()));
	}
}
