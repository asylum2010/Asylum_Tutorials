
#include <metal_stdlib>
#include <metal_atomic>

using namespace metal;

constant uint NUM_LIGHTS [[function_constant(0)]];

struct LightParticle
{
	float4 Color;
	float4 Previous;
	float4 Current;
	float4 Velocity;	// w = radius
};

struct ListHead
{
	uint4 StartAndCount;
};

struct ListNode
{
	uint4 LightIndexAndNext;
};

static_assert(sizeof(ListHead) == 16, "sizeof(ListHead) must be 16");
static_assert(sizeof(ListNode) == 16, "sizeof(ListHead) must be 16");

struct UniformData
{
	float4x4 matView;
	float4x4 matProj;
	float4x4 matViewProj;
	float4 clipPlanes;	// z = interpolation value
};

kernel void lightcull(
	texture2d<float, access::read> depthbuffer [[texture(0)]],
	device ListHead* headbuffer [[buffer(0)]],
	device ListNode* nodebuffer [[buffer(1)]],
	const device LightParticle* lightbuffer [[buffer(2)]],
	device atomic_uint& nextInsertionPoint [[buffer(3)]],
	constant UniformData& uniforms [[buffer(4)]],
	uint2 loc [[thread_position_in_grid]],
	uint2 itemID [[thread_position_in_threadgroup]],
	uint2 tileID [[threadgroup_position_in_grid]],
	uint2 tileNum [[threadgroups_per_grid]],
	uint localIndex [[thread_index_in_threadgroup]])
{
	// shared variables
	threadgroup atomic_uint TileMinZ;
	threadgroup atomic_uint TileMaxZ;
	threadgroup atomic_uint TileLightStart;
	threadgroup atomic_uint TileLightCount;
	threadgroup float4 FrustumPlanes[6];
	
	// threadgroup index
	int index = tileID.y * tileNum.x + tileID.x;
	
	if (localIndex == 0) {
		atomic_store_explicit(&TileMinZ, 0x7F7FFFFF, memory_order_relaxed);
		atomic_store_explicit(&TileMaxZ, 0, memory_order_relaxed);
		atomic_store_explicit(&TileLightStart, 0xFFFFFFFF, memory_order_relaxed);
		atomic_store_explicit(&TileLightCount, 0, memory_order_relaxed);
	}
	
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// STEP 1: calculate min/max depth in this tile
	float depth = depthbuffer.read(loc).r;
	float linearz = (uniforms.matProj[3][2] / (depth + uniforms.matProj[2][2]));
	
	float minz = min(uniforms.clipPlanes.y, linearz);
	float maxz = max(uniforms.clipPlanes.x, linearz);
	
	if (minz <= maxz) {
		atomic_fetch_min_explicit(&TileMinZ, as_type<uint>(minz), memory_order_relaxed);
		atomic_fetch_max_explicit(&TileMaxZ, as_type<uint>(maxz), memory_order_relaxed);
	}
	
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// STEP 2: calculate frustum
	if (localIndex == 0) {
		minz = as_type<float>(atomic_load_explicit(&TileMinZ, memory_order_relaxed));
		maxz = as_type<float>(atomic_load_explicit(&TileMaxZ, memory_order_relaxed));
		
		float2 step1 = (2.0f * float2(tileID)) / float2(tileNum);
		float2 step2 = (2.0f * float2(tileID + uint2(1, 1))) / float2(tileNum);
		
		FrustumPlanes[0] = float4(1, 0, 0, 1 - step1.x);	// left
		FrustumPlanes[1] = float4(-1, 0, 0, -1 + step2.x);	// right
		FrustumPlanes[2] = float4(0, 1, 0, -1 + step2.y);	// bottom
		FrustumPlanes[3] = float4(0, -1, 0, 1 - step1.y);	// top
		FrustumPlanes[4] = float4(0, 0, -1, -minz);			// near
		FrustumPlanes[5] = float4(0, 0, 1, maxz);			// far
		
		for (int i = 0; i < 4; ++i) {
			FrustumPlanes[i] = FrustumPlanes[i] * uniforms.matViewProj;
			FrustumPlanes[i] /= length(FrustumPlanes[i].xyz);
		}
		
		FrustumPlanes[4] = FrustumPlanes[4] * uniforms.matView;
		FrustumPlanes[5] = FrustumPlanes[5] * uniforms.matView;
		
		FrustumPlanes[4] /= length(FrustumPlanes[4].xyz);
		FrustumPlanes[5] /= length(FrustumPlanes[5].xyz);
	}
	
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// STEP 3: cull lights
	float4	pos;
	float	dist;
	float	radius;
	float 	alpha = uniforms.clipPlanes.z;
	
	uint	prev, next;
	uint	lightsperitem	= max(NUM_LIGHTS / 256, uint(1));
	uint	remainder		= NUM_LIGHTS % 256;
	uint	lightstart, lightend;
	
	if (localIndex < remainder) {
		++lightsperitem;
		lightstart = lightsperitem * localIndex;
	} else {
		lightstart = remainder * (lightsperitem + 1) + (localIndex - remainder) * lightsperitem;
	}
	
	lightend = min(lightstart + lightsperitem, NUM_LIGHTS);
	
	for (uint i = lightstart; i < lightend; ++i) {
		pos = mix(lightbuffer[i].Previous, lightbuffer[i].Current, alpha);
		radius = lightbuffer[i].Velocity.w;
		
		for (int j = 0; j < 6; ++j) {
			dist = dot(pos, FrustumPlanes[j]) + radius;
			
			if (dist <= 0)
				break;
		}
		
		if (dist > 0) {
			next = atomic_fetch_add_explicit(&nextInsertionPoint, 1, memory_order_relaxed);
			prev = atomic_exchange_explicit(&TileLightStart, next, memory_order_relaxed);
			
			nodebuffer[next].LightIndexAndNext = uint4(i, prev, 0, 0);
			atomic_fetch_add_explicit(&TileLightCount, 1, memory_order_relaxed);
		}
	}
	
	threadgroup_barrier(mem_flags::mem_device|mem_flags::mem_threadgroup);
	
	if (localIndex == 0) {
		uint4 result;
		
		result.x = atomic_load_explicit(&TileLightStart, memory_order_relaxed);
		result.y = atomic_load_explicit(&TileLightCount, memory_order_relaxed);
		result.z = 0;
		result.w = 0;
		
		headbuffer[index].StartAndCount = result;
	}
}
