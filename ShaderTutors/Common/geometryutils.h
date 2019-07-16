
#ifndef _GEOMETRUTILS_H_
#define _GEOMETRUTILS_H_

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

}

#endif
