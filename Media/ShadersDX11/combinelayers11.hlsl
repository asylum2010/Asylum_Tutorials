
Texture2D layer0 : register(t0);
Texture2D layer1 : register(t1);

float4 vs_main(uint vid : SV_VertexID) : SV_Position
{
	float4 vertices[4] = {
		float4(-1, -1, 0, 1),
		float4(1, -1, 0, 1),
		float4(-1, 1, 0, 1),
		float4(1, 1, 0, 1)
	};

	return vertices[vid];
}

float4 ps_main(float4 spos : SV_Position) : SV_Target
{
	float4 color0 = layer0.Load(int3(spos.xy, 0));
	float4 color1 = layer1.Load(int3(spos.xy, 0));

	return lerp(color0, color1, color1.a);
}
