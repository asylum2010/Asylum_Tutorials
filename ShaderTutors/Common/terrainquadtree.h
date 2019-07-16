
#ifndef _TERRAINQUADTREE_H_
#define _TERRAINQUADTREE_H_

#include <vector>
#include <functional>

#include "3Dmath.h"

class TerrainQuadTree
{
public:
	struct Node
	{
		int				subnodes[4];
		Math::Vector2	start;			// starting position
		float			length;			// world-space size
		int				lod;

		Node();

		inline bool IsLeaf() const {
			return (subnodes[0] == -1 && subnodes[1] == -1 && subnodes[2] == -1 && subnodes[3] == -1);
		}
	};

private:
	typedef std::vector<Node> NodeList;
	typedef std::function<void (const TerrainQuadTree::Node&)> NodeCallback;

	NodeList	nodes;
	Node		root;
	int			numlods;		// number of LOD levels
	int			meshdim;		// patch mesh resolution
	float		patchlength;	// world space patch size
	float		maxcoverage;	// any node larger than this will be subdivided
	float		screenarea;

	float CalculateCoverage(const Node& node, const Math::Matrix& proj, const Math::Vector3& eye) const;
	bool IsVisible(const Node& node, const Math::Matrix& viewproj) const;
	void InternalTraverse(const Node& node, NodeCallback callback) const;

	int FindLeaf(const Math::Vector2& point) const;
	int BuildTree(Node& node, const Math::Matrix& viewproj, const Math::Matrix& proj, const Math::Vector3& eye);

public:
	TerrainQuadTree();

	void FindSubsetPattern(int outindices[4], const Node& node);
	void Initialize(const Math::Vector2& start, float size, int lodcount, int meshsize, float patchsize, float maxgridcoverage, float screensize);
	void Rebuild(const Math::Matrix& viewproj, const Math::Matrix& proj, const Math::Vector3& eye);
	void Traverse(NodeCallback callback) const;
};

#endif
