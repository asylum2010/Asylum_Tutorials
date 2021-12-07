
#ifndef _BVH_H_
#define _BVH_H_

#include <vector>

#include "geometryutils.h"
#include "gl4ext.h"

struct OpenGLBVHNode
{
	Math::Vector3	Min;
	uint32_t		LeftOrCount;	// triangle count
	Math::Vector3	Max;
	uint32_t		RightOrStart;	// starting index
}; // 32 B total

static_assert(sizeof(OpenGLBVHNode) == 32, "sizeof(OpenGLBVHNode) must be 32 bytes");

class OpenGLBVH
{
	struct AABoxEx;

	struct Subset
	{
		uint32_t IndexStart;
		uint32_t MaterialID;

		bool operator <(const Subset& other) const {
			return (IndexStart < other.IndexStart);
		}
	};

	class BVHNode
	{
	public:
		Math::AABox Box;
		uint32_t Axis;

		BVHNode(uint32_t axis) { Axis = axis; }

		virtual ~BVHNode() {}
		virtual bool IsLeaf() const = 0;
	};

	class BVHInner : public BVHNode
	{
	public:
		BVHNode* Left;
		BVHNode* Right;

		BVHInner(uint32_t axis) : BVHNode(axis)	{ Left = Right = nullptr; }
		~BVHInner()								{ delete Left; delete Right; }

		bool IsLeaf() const override			{ return false; }
	};

	class BVHLeaf : public BVHNode
	{
	public:
		std::vector<uint32_t> TriangleIDs;

		BVHLeaf(uint32_t axis) : BVHNode(axis)	{ TriangleIDs.reserve(16); }
		bool IsLeaf() const override			{ return true; }
	};

	typedef std::pair<uint32_t, uint32_t> Triangle;

private:
	BVHNode*	root;
	Subset*		subsets;		// for materials
	GLuint		hierarchy;
	GLuint		triangles;
	uint32_t	numsubsets;		// for materials
	uint32_t	totalnodes;
	int			totaldepth;		// height of the tree
	int			maxprimitives;	// primitive count in largest leaf
	int			lastpercent;

	BVHNode* Recurse(std::vector<AABoxEx>& items, const Math::AABox& itemsbox, int depth = 0, float percent = 0.0f);
	void PopulateHeap(BVHNode* node, OpenGLBVHNode* heap, Triangle* triangles, uint32_t& nodeindex, uint32_t& triindex);
	void InternalTraverse(BVHNode* node, std::function<void (const Math::AABox&, bool)> callback);
	void UploadToGPU(OpenGLBVHNode* heap, Triangle* triangleIDs, uint32_t numtriangleIDs);

public:
	OpenGLBVH();
	~OpenGLBVH();

	void Build(OpenGLMesh* mesh, uint32_t* materialoverrides, const char* cachefile);
	void DEBUG_Traverse(std::function<void (const Math::AABox&, bool)> callback);

	inline GLuint GetHierarchy() const		{ return hierarchy; }
	inline GLuint GetTriangleIDs() const	{ return triangles; }
	inline uint32_t GetNumNodes() const		{ return totalnodes; }
};

#endif
