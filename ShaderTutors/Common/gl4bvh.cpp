
#include <iostream>
#include "gl4bvh.h"

struct OpenGLBVH::AABoxEx : private Math::AABox
{
	using Math::AABox::Min;
	using Math::AABox::Max;

	using Math::AABox::GetCenter;
	using Math::AABox::GetSize;

	uint32_t TriangleID;
};

// --- OpenGLBVH impl ---------------------------------------------------------

OpenGLBVH::OpenGLBVH()
{
	root			= nullptr;
	subsets			= nullptr;

	hierarchy		= 0;
	triangles		= 0;
	numsubsets		= 0;
	totalnodes		= 0;
	totaldepth		= 0;
	maxprimitives	= 0;
	lastpercent		= 0;
}

OpenGLBVH::~OpenGLBVH()
{
	delete root;

	GL_SAFE_DELETE_BUFFER(hierarchy);
	GL_SAFE_DELETE_BUFFER(triangles);
}

void OpenGLBVH::Build(OpenGLMesh* mesh, uint32_t* materialoverrides, const char* cachefile)
{
	FILE* infile = nullptr;

	fopen_s(&infile, cachefile, "rb");

	if (infile != nullptr) {
		uint32_t numtriangleIDs = 0;

		fread(&totalnodes, sizeof(uint32_t), 1, infile);
		fread(&numtriangleIDs, sizeof(uint32_t), 1, infile);
		fread(&totaldepth, sizeof(int), 1, infile);
		fread(&maxprimitives, sizeof(int), 1, infile);

		OpenGLBVHNode* heap = new OpenGLBVHNode[totalnodes];
		Triangle* triangles = new Triangle[numtriangleIDs];

		fread(heap, sizeof(OpenGLBVHNode), totalnodes, infile);
		fread(triangles, sizeof(Triangle), numtriangleIDs, infile);

		fclose(infile);

		// copy to GPU
		UploadToGPU(heap, triangles, numtriangleIDs);

		delete[] heap;
		delete[] triangles;

		return;
	}

#ifdef _DEBUG
	{
		char sanity;

		std::cout << "\nBVH cache not found. Build in debug mode? (y - yes, n - no): ";
		std::cin >> sanity;

		if (sanity != 'y')
			exit(1);
	}
#endif

	GL_ASSERT(mesh->GetNumBytesPerVertex() == sizeof(GeometryUtils::CommonVertex));

	// create hierarchy
	std::vector<AABoxEx> worklist;
	AABoxEx tmpbox;
	GeometryUtils::CommonVertex* vdata = nullptr;
	void* idata = nullptr;
	bool is16bit = (mesh->GetIndexType() == GL_UNSIGNED_SHORT);

	std::cout << "\nCreating BV Hierarchy...\nProgress: 0%";

	worklist.reserve(1024);

	mesh->LockVertexBuffer(0, 0, GLLOCK_READONLY, (void**)&vdata);
	mesh->LockIndexBuffer(0, 0, GLLOCK_READONLY, (void**)&idata);
	{
		for (GLuint i = 0; i < mesh->GetNumIndices(); i += 3) {
			uint32_t index0 = (is16bit ? ((uint16_t*)idata)[i + 0] : ((uint32_t*)idata)[i + 0]);
			uint32_t index1 = (is16bit ? ((uint16_t*)idata)[i + 1] : ((uint32_t*)idata)[i + 1]);
			uint32_t index2 = (is16bit ? ((uint16_t*)idata)[i + 2] : ((uint32_t*)idata)[i + 2]);

			const GeometryUtils::CommonVertex& vertex0 = vdata[index0];
			const GeometryUtils::CommonVertex& vertex1 = vdata[index1];
			const GeometryUtils::CommonVertex& vertex2 = vdata[index2];

			tmpbox.TriangleID = i;

			tmpbox.Min.x = Math::Min(vertex0.x, Math::Min(vertex1.x, vertex2.x));
			tmpbox.Min.y = Math::Min(vertex0.y, Math::Min(vertex1.y, vertex2.y));
			tmpbox.Min.z = Math::Min(vertex0.z, Math::Min(vertex1.z, vertex2.z));

			tmpbox.Max.x = Math::Max(vertex0.x, Math::Max(vertex1.x, vertex2.x));
			tmpbox.Max.y = Math::Max(vertex0.y, Math::Max(vertex1.y, vertex2.y));
			tmpbox.Max.z = Math::Max(vertex0.z, Math::Max(vertex1.z, vertex2.z));

			worklist.push_back(tmpbox);
		}
	}
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	totalnodes = 0;
	totaldepth = 0;
	maxprimitives = 0;
	lastpercent = 0;

	root = Recurse(worklist, mesh->GetBoundingBox());
	root->Box = mesh->GetBoundingBox();

	std::cout << "\b\b\b100%\n";

	if (totaldepth >= 32) {
		std::cout << "BVH depth (" << totaldepth << ") is too large!\n";
		return;
	}

	std::cout << "\nNumber of triangles: " << (mesh->GetNumIndices() / 3);
	std::cout << "\nNumber of nodes: " << totalnodes;
	std::cout << "\nNumber of subsets: " << mesh->GetNumSubsets();
	std::cout << "\nTotal depth: " << totaldepth;;
	std::cout << "\nMax primitives: " << maxprimitives << "\n";

	// sort subsets
	OpenGLAttributeRange* subsettable = mesh->GetAttributeTable();
	
	numsubsets = mesh->GetNumSubsets();
	subsets = new Subset[numsubsets];

	for (GLuint i = 0; i < mesh->GetNumSubsets(); ++i) {
		subsets[i].IndexStart = subsettable[i].IndexStart;
		subsets[i].MaterialID = ((materialoverrides == nullptr) ? i : materialoverrides[i]);
	}

	std::sort(subsets, subsets + numsubsets);

	// lay out into arrays and save to file
	OpenGLBVHNode* heap = new OpenGLBVHNode[totalnodes];
	Triangle* triangles = new Triangle[mesh->GetNumIndices() / 3];
	uint32_t verifynodes = 0, verifytriangles = 0;

	PopulateHeap(root, heap, triangles, verifynodes, verifytriangles);

	GL_ASSERT(verifynodes == totalnodes - 1);
	GL_ASSERT(verifytriangles == mesh->GetNumIndices() / 3);

	fopen_s(&infile, cachefile, "wb");

	if (infile != nullptr) {
		fwrite(&totalnodes, sizeof(uint32_t), 1, infile);
		fwrite(&verifytriangles, sizeof(uint32_t), 1, infile);
		fwrite(&totaldepth, sizeof(int), 1, infile);
		fwrite(&maxprimitives, sizeof(int), 1, infile);

		fwrite(heap, sizeof(OpenGLBVHNode), totalnodes, infile);
		fwrite(triangles, sizeof(Triangle), verifytriangles, infile);

		fclose(infile);
	}

	// copy to GPU
	UploadToGPU(heap, triangles, verifytriangles);

	delete[] heap;
	delete[] triangles;
	delete[] subsets;

	subsets = nullptr;
}

void OpenGLBVH::UploadToGPU(OpenGLBVHNode* heap, Triangle* triangleIDs, uint32_t numtriangleIDs)
{
	glGenBuffers(1, &hierarchy);
	glGenBuffers(1, &triangles);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, hierarchy);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, totalnodes * sizeof(OpenGLBVHNode), heap, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangles);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, numtriangleIDs * sizeof(Triangle), triangleIDs, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

OpenGLBVH::BVHNode* OpenGLBVH::Recurse(std::vector<AABoxEx>& items, const Math::AABox& itemsbox, int depth, float percent)
{
	auto reportPercent = [&](float currpercent) {
		int ipercent = (int)currpercent;

		if (lastpercent != ipercent) {
			if (lastpercent < 10)
				std::cout << "\b\b" << ipercent << "%";
			else
				std::cout << "\b\b\b" << ipercent << "%";

			lastpercent = ipercent;
		}
	};

	Math::AABox bestleft, bestright;
	Math::Vector3 center, size;
	float percentspan = 11.0f / powf(3.0f, (float)depth);

	++totalnodes;
	totaldepth = Math::Max(totaldepth, depth);

	// create leaf for less than 4 triangles
	if (items.size() < 4) {
		BVHLeaf* leaf = new BVHLeaf(0);

		for (const AABoxEx& box : items)
			leaf->TriangleIDs.push_back(box.TriangleID);

		maxprimitives = Math::Max<int>(maxprimitives, (int)items.size());
		return leaf;
	}
	
	itemsbox.GetSize(size);
	
	// surface area heuristic
	float mincost = (float)items.size() * (size.x * size.y + size.y * size.z + size.z * size.x);
	float bestsplit = FLT_MAX;
	float start, stop, step;
	float percentstart, percentstep;
	int bestcountleft, bestcountright;
	int bestaxis = -1;

	// test all axes
	for (int i = 0; i < 3; ++i) {
		start = itemsbox.Min[i];
		stop = itemsbox.Max[i];

		if (stop - start < 1e-4f)
			continue;

		step = (stop - start) / (1024.0f / (depth + 1));

		percentstart = percent + i * percentspan;
		percentstep = (step * percentspan) / (stop - start - 2 * step);

		// find best splitting point
		for (float testsplit = start + step; testsplit < stop - step; testsplit += step) {
			Math::AABox left, right;
			float surfleft, surfright;
			int countleft = 0, countright = 0;

			reportPercent(percentstart);
			percentstart += percentstep;

			for (const AABoxEx& box : items) {
				box.GetCenter(center);

				if (center[i] < testsplit) {
					// put into left box
					left.Add(box.Min);
					left.Add(box.Max);

					++countleft;
				} else {
					// put into right box
					right.Add(box.Min);
					right.Add(box.Max);

					++countright;
				}
			}

			if (countleft <= 1 || countright <= 1)
				continue;

			// surface area heuristic
			left.GetSize(size);
			surfleft = size.x * size.y + size.y * size.z + size.z * size.x;

			right.GetSize(size);
			surfright = size.x * size.y + size.y * size.z + size.z * size.x;

			float totalcost = surfleft * countleft + surfright * countright;

			if (totalcost < mincost) {
				// found a better split
				mincost = totalcost;
				bestsplit = testsplit;
				bestaxis = i;

				bestcountleft = countleft;
				bestcountright = countright;

				bestleft = left;
				bestright = right;
			}
		}
	}

	if (bestaxis == -1) {
		// no split found, create leaf
		BVHLeaf* leaf = new BVHLeaf(0);

		for (const AABoxEx& box : items)
			leaf->TriangleIDs.push_back(box.TriangleID);

		maxprimitives = Math::Max<int>(maxprimitives, (int)items.size());
		return leaf;
	}

	// distribute triangles and create inner node
	std::vector<AABoxEx> leftitems, rightitems;

	leftitems.reserve(bestcountleft);
	rightitems.reserve(bestcountright);

	for (const AABoxEx& box : items) {
		box.GetCenter(center);

		if (center[bestaxis] < bestsplit) {
			leftitems.push_back(box);
		} else {
			rightitems.push_back(box);
		}
	}

	BVHInner* inner = new BVHInner(bestaxis);

	inner->Left = Recurse(leftitems, bestleft, depth + 1, percent + 3 * percentspan);
	inner->Left->Box = bestleft;

	inner->Right = Recurse(rightitems, bestright, depth + 1, percent + 6 * percentspan);
	inner->Right->Box = bestright;

	return inner;
}

void OpenGLBVH::PopulateHeap(BVHNode* node, OpenGLBVHNode* heap, Triangle* triangles, uint32_t& nodeindex, uint32_t& triindex)
{
	uint32_t thisnode = nodeindex;

	heap[thisnode].Min = node->Box.Min;
	heap[thisnode].Max = node->Box.Max;

	if (node->IsLeaf()) {
		BVHLeaf* leaf = (BVHLeaf*)node;
		uint32_t count = (uint32_t)leaf->TriangleIDs.size();

		heap[thisnode].LeftOrCount = 0x80000000 | count;
		heap[thisnode].RightOrStart = triindex;

		for (uint32_t triID : leaf->TriangleIDs) {
			auto subset = std::upper_bound(subsets, subsets + numsubsets, triID, [](const uint32_t& v, const Subset& s) -> bool {
				return (v < s.IndexStart);
			});

			assert(subset != subsets);
			--subset;

			triangles[triindex].first = triID;
			triangles[triindex].second = subset->MaterialID;

			++triindex;
		}
	} else {
		BVHInner* inner = (BVHInner*)node;
		uint32_t leftnode, rightnode;

		leftnode = ++nodeindex;
		PopulateHeap(inner->Left, heap, triangles, nodeindex, triindex);

		rightnode = ++nodeindex;
		PopulateHeap(inner->Right, heap, triangles, nodeindex, triindex);

		uint32_t axismask = (node->Axis << 30) & 0xC0000000;

		heap[thisnode].LeftOrCount = leftnode;
		heap[thisnode].RightOrStart = axismask | rightnode;
	}
}

void OpenGLBVH::InternalTraverse(BVHNode* node, std::function<void (const Math::AABox&, bool)> callback)
{
	if (node->IsLeaf()) {
		callback(node->Box, true);
	} else {
		BVHInner* inner = (BVHInner*)node;

		InternalTraverse(inner->Left, callback);
		InternalTraverse(inner->Right, callback);

		callback(node->Box, false);
	}
}

void OpenGLBVH::DEBUG_Traverse(std::function<void (const Math::AABox&, bool)> callback)
{
	if (root != nullptr && callback != nullptr)
		InternalTraverse(root, callback);
}
