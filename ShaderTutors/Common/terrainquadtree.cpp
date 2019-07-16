
#include "terrainquadtree.h"

#define CHOOPY_SCALE_CORRECTION	1.35f	// because frustum culling gives false negatives

TerrainQuadTree::Node::Node()
{
	subnodes[0] = -1;
	subnodes[1] = -1;
	subnodes[2] = -1;
	subnodes[3] = -1;

	lod = 0;
	length = 0;
}

TerrainQuadTree::TerrainQuadTree()
{
	numlods		= 0;
	meshdim		= 0;
	patchlength	= 0;
	maxcoverage	= 0;
	screenarea	= 0;
}

void TerrainQuadTree::Initialize(const Math::Vector2& start, float size, int lodcount, int meshsize, float patchsize, float maxgridcoverage, float screensize)
{
	root.start	= start;
	root.length	= size;

	numlods		= lodcount;
	meshdim		= meshsize;
	patchlength	= patchsize;
	maxcoverage	= maxgridcoverage;
	screenarea	= screensize;
}

void TerrainQuadTree::Rebuild(const Math::Matrix& viewproj, const Math::Matrix& proj, const Math::Vector3& eye)
{
	nodes.clear();

	BuildTree(root, viewproj, proj, eye);
}

int TerrainQuadTree::BuildTree(Node& node, const Math::Matrix& viewproj, const Math::Matrix& proj, const Math::Vector3& eye)
{
	if (!IsVisible(node, viewproj))
		return -1;

	float coverage = CalculateCoverage(node, proj, eye);
	bool visible = true;

	if (coverage > maxcoverage && node.length > patchlength) {
		Node subnodes[4];

		subnodes[0].start = node.start;
		subnodes[1].start = { node.start.x + 0.5f * node.length, node.start.y };
		subnodes[2].start = { node.start.x + 0.5f * node.length, node.start.y + 0.5f * node.length };
		subnodes[3].start = { node.start.x, node.start.y + 0.5f * node.length };

		subnodes[0].length = subnodes[1].length = subnodes[2].length = subnodes[3].length = 0.5f * node.length;

		node.subnodes[0] = BuildTree(subnodes[0], viewproj, proj, eye);
		node.subnodes[1] = BuildTree(subnodes[1], viewproj, proj, eye);
		node.subnodes[2] = BuildTree(subnodes[2], viewproj, proj, eye);
		node.subnodes[3] = BuildTree(subnodes[3], viewproj, proj, eye);

		visible = !node.IsLeaf();
	}

	if (visible) {
		int lod = 0;

		for (lod = 0; lod < numlods - 1; ++lod) {
			if (coverage > maxcoverage)
				break;

			coverage *= 4.0f;
		}

		node.lod = Math::Min(lod, numlods - 2);
	} else {
		return -1;
	}

	int position = (int)nodes.size();
	nodes.push_back(node);

	return position;
}

bool TerrainQuadTree::IsVisible(const Node& node, const Math::Matrix& viewproj) const
{
	Math::AABox box;
	Math::Vector4 planes[6];
	float length = node.length;

	// NOTE: depends on choopy scale
	box.Add(node.start.x - CHOOPY_SCALE_CORRECTION, -0.01f, node.start.y - CHOOPY_SCALE_CORRECTION);
	box.Add(node.start.x + length + CHOOPY_SCALE_CORRECTION, 0.01f, node.start.y + length + CHOOPY_SCALE_CORRECTION);

	Math::FrustumPlanes(planes, viewproj);
	int result = FrustumIntersect(planes, box);

	return (result > 0);
}

int TerrainQuadTree::FindLeaf(const Math::Vector2& point) const
{
	int index = -1;
	int size = (int)nodes.size();

	Node node = nodes[size - 1];

	while (!node.IsLeaf()) {
		bool found = false;

		for (int i = 0; i < 4; ++i) {
			index = node.subnodes[i];

			if (index == -1)
				continue;

			Node subnode = nodes[index];

			if (point.x >= subnode.start.x && point.x <= subnode.start.x + subnode.length &&
				point.y >= subnode.start.y && point.y <= subnode.start.y + subnode.length)
			{
				node = subnode;
				found = true;

				break;
			}
		}

		if (!found)
			return -1;
	}

	return index;
}

void TerrainQuadTree::FindSubsetPattern(int outindices[4], const Node& node)
{
	outindices[0] = 0;
	outindices[1] = 0;
	outindices[2] = 0;
	outindices[3] = 0;

	// NOTE: bottom: +Z, top: -Z
	Math::Vector2 point_left	= { node.start.x - 0.5f * patchlength, node.start.y + 0.5f * node.length };
	Math::Vector2 point_right	= { node.start.x + node.length + 0.5f * patchlength, node.start.y + 0.5f * node.length };
	Math::Vector2 point_bottom	= { node.start.x + 0.5f * node.length, node.start.y + node.length + 0.5f * patchlength };
	Math::Vector2 point_top		= { node.start.x + 0.5f * node.length, node.start.y - 0.5f * patchlength };

	int adjacency[4] = {
		FindLeaf(point_left),
		FindLeaf(point_right),
		FindLeaf(point_bottom),
		FindLeaf(point_top)
	};

	// NOTE: from nVidia (chooses the closest LOD degrees)
	for (int i = 0; i < 4; ++i) {
		if (adjacency[i] != -1 && nodes[adjacency[i]].length > node.length * 0.999f) {
			const Node& adj = nodes[adjacency[i]];
			float scale = adj.length / node.length * (meshdim >> node.lod) / (meshdim >> adj.lod);
		
			if (scale > 3.999f)
				outindices[i] = 2;
			else if (scale > 1.999f)
				outindices[i] = 1;
		}
	}
}

void TerrainQuadTree::Traverse(NodeCallback callback) const
{
	if (nodes.size() == 0)
		return;

	InternalTraverse(nodes.back(), callback);
}

float TerrainQuadTree::CalculateCoverage(const Node& node, const Math::Matrix& proj, const Math::Vector3& eye) const
{
	const static Math::Vector2 samples[16] = {
		{ 0, 0 },
		{ 0, 1 },
		{ 1, 0 },
		{ 1, 1 },
		{ 0.5f, 0.333f },
		{ 0.25f, 0.667f },
		{ 0.75f, 0.111f },
		{ 0.125f, 0.444f },
		{ 0.625f, 0.778f },
		{ 0.375f, 0.222f },
		{ 0.875f, 0.556f },
		{ 0.0625f, 0.889f },
		{ 0.5625f, 0.037f },
		{ 0.3125f, 0.37f },
		{ 0.8125f, 0.704f },
		{ 0.1875f, 0.148f },
	};

	Math::Vector3 test;
	Math::Vector3 vdir;
	float length		= node.length;
	float gridlength	= length / meshdim;
	float worldarea		= gridlength * gridlength;
	float maxprojarea	= 0;

	// NOTE: from nVidia (sample patch at given points and estimate coverage)
	for (int i = 0; i < 16; ++i) {
		test.x = (node.start.x - CHOOPY_SCALE_CORRECTION) + (length + 2 * CHOOPY_SCALE_CORRECTION) * samples[i].x;
		test.y = 0;
		test.z = (node.start.y - CHOOPY_SCALE_CORRECTION) + (length + 2 * CHOOPY_SCALE_CORRECTION) * samples[i].y;

		Math::Vec3Subtract(vdir, test, eye);

		float dist = Math::Vec3Length(vdir);
		float projarea = (worldarea * proj._11 * proj._22) / (dist * dist);

		if (maxprojarea < projarea)
			maxprojarea = projarea;
	}

	return maxprojarea * screenarea * 0.25f;
}

void TerrainQuadTree::InternalTraverse(const Node& node, NodeCallback callback) const
{
	if (node.IsLeaf()) {
		callback(node);
	} else {
		if (node.subnodes[0] != -1)
			InternalTraverse(nodes[node.subnodes[0]], callback);

		if (node.subnodes[1] != -1)
			InternalTraverse(nodes[node.subnodes[1]], callback);

		if (node.subnodes[2] != -1)
			InternalTraverse(nodes[node.subnodes[2]], callback);

		if (node.subnodes[3] != -1)
			InternalTraverse(nodes[node.subnodes[3]], callback);
	}
}
