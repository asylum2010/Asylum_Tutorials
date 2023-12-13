
#ifndef _GEOMETRUTILS_H_
#define _GEOMETRUTILS_H_

#include <vector>
#include <functional>

#include "orderedarray.hpp"
#include "3Dmath.h"

namespace GeometryUtils {

// --- Structures -------------------------------------------------------------

struct CommonVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

struct TBNVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float tx, ty, tz;
	float bx, by, bz;
};

struct PositionVertex
{
	float x, y, z;
};

struct BillboardVertex
{
	float x, y, z;
	uint32_t color;
	float u, v;
};

struct Edge
{
	mutable Math::Vector3 v1, v2;	// edge vertices
	mutable Math::Vector3 n1, n2;	// adjacent triangle normals
	uint32_t i1, i2;				// first triangle edge indices
	mutable uint32_t other;			// second triangle starting index

	Edge();

	inline bool operator <(const Edge& other) const {
		return ((i1 < other.i1) || (i1 == other.i1 && i2 < other.i2));
	}
};

typedef OrderedArray<Edge> EdgeSet;

struct FrustumEdge
{
	Math::Vector3 v0;
	Math::Vector3 v1;

	FrustumEdge() = default;
	FrustumEdge(const FrustumEdge&) = default;

	FrustumEdge& operator =(const FrustumEdge&) = default;

	FrustumEdge(const Math::Vector4& p0, const Math::Vector4& p1) {
		v0 = (Math::Vector3&)p0;
		v1 = (Math::Vector3&)p1;
	}
};

struct FrustumTriangle
{
	Math::Vector3 v0;
	Math::Vector3 v1;
	Math::Vector3 v2;

	FrustumTriangle() = default;
	FrustumTriangle(const FrustumTriangle&) = default;

	FrustumTriangle& operator =(const FrustumTriangle&) = default;

	FrustumTriangle(const Math::Vector4& p0, const Math::Vector4& p1, const Math::Vector4& p2) {
		v0 = (Math::Vector3&)p0;
		v1 = (Math::Vector3&)p1;
		v2 = (Math::Vector3&)p2;
	}
};

// --- Functions --------------------------------------------------------------

void AccumulateTangentFrame(TBNVertex* vdata, uint32_t i1, uint32_t i2, uint32_t i3);
void OrthogonalizeTangentFrame(TBNVertex& vert);

void CreatePlane(CommonVertex* outvdata, uint32_t* outidata, float width, float depth);
void CreateBox(CommonVertex* outvdata, uint32_t* outidata, float width, float height, float depth);
void CreateSphere(CommonVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments);
void CreateCapsule(CommonVertex* outvdata, uint32_t* outidata, float length, float radius, uint16_t vsegments, uint16_t hsegments);
void CreateLShape(CommonVertex* outvdata, uint32_t* outidata, float basewidth, float innerwidth, float height, float basedepth, float innerdepth);

void Create2MBox(PositionVertex* outvdata, uint32_t* outidata, float width, float height, float depth);
void Create2MSphere(PositionVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments);
void Create2MLShape(PositionVertex* outvdata, uint32_t* outidata, float basewidth, float innerwidth, float height, float basedepth, float innerdepth);

void GenerateEdges(EdgeSet& out, const PositionVertex* vertices, const uint32_t* indices, uint32_t numindices);
void GenerateGSAdjacency(uint32_t* outidata, const uint32_t* idata, uint32_t numindices);
void GenerateTangentFrame(TBNVertex* outvdata, const CommonVertex* vdata, uint32_t numvertices, const uint32_t* indices, uint32_t numindices);

void NumVerticesIndicesSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments);
void NumVerticesIndicesCapsule(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments);
void NumVerticesIndices2MSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments);

bool PointInFrustum(const Math::Vector4 frustumplanes[6], const Math::Vector3& point);
bool SegmentIntersectTriangle(Math::Vector3& result, const FrustumEdge& segment, const FrustumTriangle& triangle);
bool SegmentIntersectFrustum(const std::vector<FrustumTriangle>& frustumtriangles, const FrustumEdge& segment, std::function<void (const Math::Vector3&)> callback);
bool SegmentIntersectAABox(const Math::AABox& box, const FrustumEdge& segment, std::function<void (const Math::Vector3&)> callback);
bool RayIntersectAABox(const Math::AABox& box, const Math::Vector3& p, const Math::Vector3& dir, std::function<void (float)> callback);

void SegmentateFrustum(std::vector<FrustumEdge>& result, const Math::Matrix& viewprojinv);
void TriangulateFrustum(std::vector<FrustumTriangle>& result, const Math::Matrix& viewprojinv);
void FrustumIntersectAABox(std::vector<Math::Vector3>& result, const Math::Matrix& viewproj, const Math::AABox& box);
void LightVolumeIntersectAABox(std::vector<Math::Vector3>& points, const Math::Vector3& lightdir, const Math::AABox& box);
void CalculateAABoxFromPoints(Math::AABox& out, const std::vector<Math::Vector3>& points, const Math::Matrix& viewproj);

}

#endif
