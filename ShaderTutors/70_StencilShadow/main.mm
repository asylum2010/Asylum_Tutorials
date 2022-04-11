
#import <MetalKit/MetalKit.h>
#import <iostream>

#include "application.h"
#include "mtlext.h"
#include "basiccamera.h"
#include "geometryutils.h"

#include <vector>

#define NUM_QUEUED_FRAMES	3
#define MANIFOLD_EPSILON	8e-3f	// to avoid zfight

// helper macros
#define TITLE				"Shader sample 70: Stencil shadow volumes"

// sample structures
struct CombinedUniformData
{
	// byte offset 0
	struct BlinnphongUniformData {
		Math::Matrix world;
		Math::Matrix worldinv;
		Math::Matrix viewproj;
		Math::Vector4 lightpos;
		Math::Vector4 eyepos;
		Math::Vector4 uvscale;
	} blinnphonguniforms;	// 240 B
	
	Math::Vector4 padding1[1];	// 16 B
	
	// byte offset 256
	struct SimpleColorUniformData {
		Math::Matrix world;
		Math::Matrix viewproj;
		Math::Vector4 color;
	} simplecoloruniforms;	// 144 B
	
	Math::Vector4 padding2[7];	// 112 B
};

static_assert(sizeof(CombinedUniformData) % 256 == 0, "Uniform data must be 256 byte aligned!");

// sample classes
class ShadowCaster
{
	typedef GeometryUtils::EdgeSet EdgeSet;
	typedef std::vector<GeometryUtils::Edge> EdgeList;

private:
	Math::Matrix		transform;
	EdgeSet				edges;
	EdgeList			silhouette;

	MetalMesh*			mesh;			// for rendering
	MetalMesh*			manifold;		// for shadow generation
	id<MTLBuffer>		volumevertices;
	id<MTLBuffer>		volumeindices;

	uint32_t			numvolumevertices;
	uint32_t			numvolumeindices;
	
	uint32_t			maxvolumevertices;
	uint32_t			maxvolumeindices;

	void GenerateSilhouette(const Math::Vector4& position);

public:
	ShadowCaster();
	~ShadowCaster();

	void CreateGeometry(std::function<void(MetalMesh**, MetalMesh**)> callback);
	void ExtrudeSilhouette(uint8_t flight, const Math::Vector4& position);
	void SetTransform(const Math::Matrix& transform);
	void UploadToVRAM(id<MTLCommandBuffer> commandbuffer);
	void DeleteStagingBuffers();

	inline const Math::Matrix& GetTransform() const		{ return transform; }
	inline const EdgeList& GetSilhouette() const		{ return silhouette; }

	inline MetalMesh* GetMesh()							{ return mesh; }
	inline MetalMesh* GetManifold()						{ return manifold; }

	inline id<MTLBuffer> GetVolumeVertices()			{ return volumevertices; }
	inline id<MTLBuffer> GetVolumeIndices()				{ return volumeindices; }

	inline uint32_t GetNumVolumeVertices() const		{ return numvolumevertices; }
	inline uint32_t GetNumVolumeIndices() const			{ return numvolumeindices; }
	
	inline uint32_t GetVolumeVertexPitch() const		{ return maxvolumevertices * sizeof(Math::Vector4); }
	inline uint32_t GetVolumeIndexPitch() const			{ return maxvolumeindices * sizeof(uint32_t); }
};

// sample variables
Application*				app					= nullptr;
MTKView*					mtkview				= nil;

id<MTLDevice>				device				= nil;
id<MTLCommandQueue>			commandqueue		= nil;
id<MTLLibrary>				defaultlibrary		= nil;

id<MTLRenderPipelineState>	blackstate			= nil;	// render everything with black, fill depth buffer
id<MTLRenderPipelineState>	yellowstate			= nil;	// for silhouette and volume render
id<MTLRenderPipelineState>	shadowstate			= nil;	// render shadow volumes with Carmack's Reverse
id<MTLRenderPipelineState> 	blinnphongstate		= nil;	// mask out lighting where stencil is nonzero

id<MTLDepthStencilState>	defaultdepthstate	= nil;	// for normal draw
id<MTLDepthStencilState>	zfailstate			= nil;	// Carmack's Reverse
id<MTLDepthStencilState>	maskingstate		= nil;	// for masking out lighting

id<MTLTexture>				wood				= nil;
id<MTLTexture>				marble				= nil;
id<MTLTexture>				helptext			= nil;

ShadowCaster*				shadowcasters[3]	= { nullptr };
BasicCamera					camera;
BasicCamera					light;
MetalScreenQuad*			screenquad			= nullptr;
MetalMesh*					box					= nullptr;
MetalDynamicRingBuffer*		uniformbuffer		= nullptr;
Math::Matrix 				viewproj;
dispatch_semaphore_t		flightsema			= 0;
uint8_t 					currentframe		= 0;
bool						drawvolume			= false;
bool						drawsilhouette		= false;

// --- Sample classes impl ----------------------------------------------------

ShadowCaster::ShadowCaster()
{
	mesh				= nullptr;
	manifold			= nullptr;
	volumevertices		= nil;
	volumeindices		= nil;
	numvolumevertices	= 0;
	numvolumeindices	= 0;
}

ShadowCaster::~ShadowCaster()
{
	volumevertices = nil;
	volumeindices = nil;
	
	delete mesh;
	delete manifold;
}

void ShadowCaster::CreateGeometry(std::function<void(MetalMesh**, MetalMesh**)> callback)
{
	callback(&mesh, &manifold);

	GeometryUtils::PositionVertex* vdata = (GeometryUtils::PositionVertex*)manifold->GetVertexBufferPointer();
	uint32_t* idata = (uint32_t*)manifold->GetIndexBufferPointer();
	{
		GeometryUtils::GenerateEdges(edges, vdata, idata, manifold->GetNumPolygons() * 3);
	}

	// worst case scenario
	maxvolumevertices = 18 * manifold->GetNumPolygons();
	maxvolumeindices = 18 * manifold->GetNumPolygons();
	
	volumevertices = [device newBufferWithLength:NUM_QUEUED_FRAMES * maxvolumevertices * sizeof(Math::Vector4) options:MTLResourceStorageModeShared|MTLResourceOptionCPUCacheModeDefault];

	volumeindices = [device newBufferWithLength:NUM_QUEUED_FRAMES * maxvolumeindices * sizeof(uint32_t) options:MTLResourceStorageModeShared|MTLResourceOptionCPUCacheModeDefault];
}

void ShadowCaster::GenerateSilhouette(const Math::Vector4& position)
{
	Math::Matrix	worldinv;
	Math::Vector3	lp;
	float			dist1, dist2;

	// edges are in object space
	Math::MatrixInverse(worldinv, transform);
	Math::Vec3TransformCoord(lp, (const Math::Vector3&)position, worldinv);

	silhouette.clear();
	silhouette.reserve(50);

	for (size_t i = 0; i < edges.Size(); ++i) {
		const GeometryUtils::Edge& edge = edges[i];

		if (edge.other != UINT_MAX) {
			dist1 = Math::Vec3Dot(lp, edge.n1) - Math::Vec3Dot(edge.v1, edge.n1);
			dist2 = Math::Vec3Dot(lp, edge.n2) - Math::Vec3Dot(edge.v1, edge.n2);

			if ((dist1 < 0) != (dist2 < 0)) {
				// silhouette edge
				if (silhouette.capacity() <= silhouette.size())
					silhouette.reserve(silhouette.size() * 2);

				if (dist1 < 0) {
					std::swap(edge.v1, edge.v2);
					std::swap(edge.n1, edge.n2);
				}

				silhouette.push_back(edge);
			}
		}
	}
}

void ShadowCaster::ExtrudeSilhouette(uint8_t flight, const Math::Vector4& position)
{
	// find silhouette first
	GenerateSilhouette(position);

	// then extrude
	Math::Vector4*	vdata = nullptr;
	uint32_t*		idata = nullptr;
	Math::Matrix	worldinv;
	Math::Vector3	lp;

	vdata = (Math::Vector4*)[volumevertices contents];
	idata = (uint32_t*)[volumeindices contents];
	
	vdata += flight * maxvolumevertices;
	idata += flight * maxvolumeindices;

	Math::MatrixInverse(worldinv, transform);
	Math::Vec3TransformCoord(lp, (const Math::Vector3&)position, worldinv);

	// boundary
	for (size_t i = 0; i < silhouette.size(); ++i) {
		const GeometryUtils::Edge& edge = silhouette[i];

		vdata[i * 4 + 0] = Math::Vector4(edge.v1, 1);
		vdata[i * 4 + 1] = Math::Vector4(edge.v1 - lp, 0);
		vdata[i * 4 + 2] = Math::Vector4(edge.v2, 1);
		vdata[i * 4 + 3] = Math::Vector4(edge.v2 - lp, 0);

		idata[i * 6 + 0] = (uint32_t)(i * 4 + 0);
		idata[i * 6 + 1] = (uint32_t)(i * 4 + 1);
		idata[i * 6 + 2] = (uint32_t)(i * 4 + 2);

		idata[i * 6 + 3] = (uint32_t)(i * 4 + 2);
		idata[i * 6 + 4] = (uint32_t)(i * 4 + 1);
		idata[i * 6 + 5] = (uint32_t)(i * 4 + 3);
	}

	numvolumevertices = (uint32_t)(silhouette.size() * 4);
	numvolumeindices = (uint32_t)(silhouette.size() * 6);

	// front & back cap
	Math::Vector3 a, b, n;
	GeometryUtils::PositionVertex* mvdata = nullptr;
	uint32_t* midata = nullptr;
	float dist;

	mvdata = (GeometryUtils::PositionVertex*)manifold->GetVertexBufferPointer();
	midata = (uint32_t*)manifold->GetIndexBufferPointer();
	{
		// only add triangles that face the light
		for (uint32_t i = 0; i < manifold->GetNumPolygons() * 3; i += 3) {
			uint32_t i1 = midata[i + 0];
			uint32_t i2 = midata[i + 1];
			uint32_t i3 = midata[i + 2];

			const GeometryUtils::PositionVertex& v1 = *(mvdata + i1);
			const GeometryUtils::PositionVertex& v2 = *(mvdata + i2);
			const GeometryUtils::PositionVertex& v3 = *(mvdata + i3);

			Math::Vec3Subtract(a, (const Math::Vector3&)v2, (const Math::Vector3&)v1);
			Math::Vec3Subtract(b, (const Math::Vector3&)v3, (const Math::Vector3&)v1);
			Math::Vec3Cross(n, a, b);

			dist = Math::Vec3Dot(n, lp) - Math::Vec3Dot(n, (const Math::Vector3&)v1);

			if (dist > 0) {
				vdata[numvolumevertices + 0] = Math::Vector4(v1.x, v1.y, v1.z, 1);
				vdata[numvolumevertices + 1] = Math::Vector4(v2.x, v2.y, v2.z, 1);
				vdata[numvolumevertices + 2] = Math::Vector4(v3.x, v3.y, v3.z, 1);

				vdata[numvolumevertices + 3] = Math::Vector4(v1.x - lp.x, v1.y - lp.y, v1.z - lp.z, 0);
				vdata[numvolumevertices + 4] = Math::Vector4(v3.x - lp.x, v3.y - lp.y, v3.z - lp.z, 0);
				vdata[numvolumevertices + 5] = Math::Vector4(v2.x - lp.x, v2.y - lp.y, v2.z - lp.z, 0);
				
				idata[numvolumeindices + 0] = numvolumevertices;
				idata[numvolumeindices + 1] = numvolumevertices + 1;
				idata[numvolumeindices + 2] = numvolumevertices + 2;

				idata[numvolumeindices + 3] = numvolumevertices + 3;
				idata[numvolumeindices + 4] = numvolumevertices + 4;
				idata[numvolumeindices + 5] = numvolumevertices + 5;

				numvolumevertices += 6;
				numvolumeindices += 6;
			}
		}
	}
}

void ShadowCaster::SetTransform(const Math::Matrix& transform)
{
	this->transform = transform;
}

void ShadowCaster::UploadToVRAM(id<MTLCommandBuffer> commandbuffer)
{
	if (mesh != nullptr)
		mesh->UploadToVRAM(commandbuffer);
}

void ShadowCaster::DeleteStagingBuffers()
{
	if (mesh != nullptr)
		mesh->DeleteStagingBuffers();
}

// --- Sample impl ------------------------------------------------------------

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();
	
	// NOTE: must query these here, as app->Run() starts the entire event loop
	mtkview = (MTKView*)CFBridgingRelease(app->GetSwapChain());
	device = CFBridgingRelease(app->GetLogicalDevice());
	
	flightsema = dispatch_semaphore_create(NUM_QUEUED_FRAMES);
	commandqueue = [device newCommandQueue];
	defaultlibrary = [device newDefaultLibrary];
	
	// load mesh
	NSString* path = [[NSBundle mainBundle] pathForResource:@"box" ofType:@"qm"];
	box = MetalMesh::LoadFromQM(device, commandqueue, [path UTF8String]);

	if (box == nullptr) {
		NSLog(@"Error: Could not load mesh!");
		return false;
	}
	
	// load textures
	path = [[NSBundle mainBundle] pathForResource:@"wood2" ofType:@"jpg"];
	wood = MetalCreateTextureFromFile(device, commandqueue, [path UTF8String], true);
	
	path = [[NSBundle mainBundle] pathForResource:@"marble" ofType:@"dds"];
	marble = MetalCreateTextureFromDDS(device, commandqueue, [path UTF8String], true);
	
	// create shadow casters
	Math::Matrix transform;

	shadowcasters[0] = new ShadowCaster();
	shadowcasters[1] = new ShadowCaster();
	shadowcasters[2] = new ShadowCaster();

	Math::MatrixTranslation(transform, -1.5f, 0.55f, -1.2f);
	shadowcasters[0]->SetTransform(transform);

	Math::MatrixTranslation(transform, 1.0f, 0.55f, -0.7f);
	shadowcasters[1]->SetTransform(transform);

	Math::MatrixTranslation(transform, 0.0f, 0.55f, 1.0f);
	shadowcasters[2]->SetTransform(transform);

	MTLVertexDescriptor* manifoldlayout = [[MTLVertexDescriptor alloc] init];
	
	manifoldlayout.attributes[0].bufferIndex = 0;
	manifoldlayout.attributes[0].format = MTLVertexFormatFloat3;
	manifoldlayout.attributes[0].offset = 0;
	
	manifoldlayout.layouts[0].stride = sizeof(GeometryUtils::PositionVertex);
	manifoldlayout.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	
	MTLVertexDescriptor* shadowlayout = [[MTLVertexDescriptor alloc] init];
	
	shadowlayout.attributes[0].bufferIndex = 0;
	shadowlayout.attributes[0].format = MTLVertexFormatFloat4;
	shadowlayout.attributes[0].offset = 0;
	
	shadowlayout.layouts[0].stride = sizeof(Math::Vector4);
	shadowlayout.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	
	shadowcasters[0]->CreateGeometry([&](MetalMesh** mesh, MetalMesh** manifold) {
		// box
		(*mesh) = new MetalMesh(device, 24, 36, sizeof(GeometryUtils::CommonVertex), nil, 0, MTL_MESH_32BIT);
		(*manifold) = new MetalMesh(device, 8, 36, sizeof(GeometryUtils::PositionVertex), manifoldlayout, MTL_MESH_32BIT|MTL_MESH_SYSTEMMEM);
		
		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		vdata1 = (GeometryUtils::CommonVertex*)(*mesh)->GetVertexBufferPointer();
		idata = (uint32_t*)(*mesh)->GetIndexBufferPointer();
		{
			GeometryUtils::CreateBox(vdata1, idata, 1, 1, 1);
		}

		vdata2 = (GeometryUtils::PositionVertex*)(*manifold)->GetVertexBufferPointer();
		idata = (uint32_t*)(*manifold)->GetIndexBufferPointer();
		{
			GeometryUtils::Create2MBox(vdata2, idata, (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON));
		}
	});

	shadowcasters[1]->CreateGeometry([&](MetalMesh** mesh, MetalMesh** manifold) {
		// sphere
		uint32_t numverts, numinds, num2mverts, num2minds;

		GeometryUtils::NumVerticesIndicesSphere(numverts, numinds, 32, 32);
		GeometryUtils::NumVerticesIndices2MSphere(num2mverts, num2minds, 32, 32);

		(*mesh) = new MetalMesh(device, numverts, numinds, sizeof(GeometryUtils::CommonVertex), nil, 0, MTL_MESH_32BIT);
		(*manifold) = new MetalMesh(device, num2mverts, num2minds, sizeof(GeometryUtils::PositionVertex), manifoldlayout, MTL_MESH_32BIT|MTL_MESH_SYSTEMMEM);
		
		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		vdata1 = (GeometryUtils::CommonVertex*)(*mesh)->GetVertexBufferPointer();
		idata = (uint32_t*)(*mesh)->GetIndexBufferPointer();
		{
			GeometryUtils::CreateSphere(vdata1, idata, 0.5f, 32, 32);
		}

		vdata2 = (GeometryUtils::PositionVertex*)(*manifold)->GetVertexBufferPointer();
		idata = (uint32_t*)(*manifold)->GetIndexBufferPointer();
		{
			GeometryUtils::Create2MSphere(vdata2, idata, 0.5f - MANIFOLD_EPSILON, 32, 32);
		}
	});

	shadowcasters[2]->CreateGeometry([&](MetalMesh** mesh, MetalMesh** manifold) {
		// L-shape
		(*mesh) = new MetalMesh(device, 36, 60, sizeof(GeometryUtils::CommonVertex), nil, 0, MTL_MESH_32BIT);
		(*manifold) = new MetalMesh(device, 12, 60, sizeof(GeometryUtils::PositionVertex), manifoldlayout, MTL_MESH_32BIT|MTL_MESH_SYSTEMMEM);

		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		vdata1 = (GeometryUtils::CommonVertex*)(*mesh)->GetVertexBufferPointer();
		idata = (uint32_t*)(*mesh)->GetIndexBufferPointer();
		{
			GeometryUtils::CreateLShape(vdata1, idata, 1.5f, 1.0f, 1, 1.2f, 0.6f);
		}

		vdata2 = (GeometryUtils::PositionVertex*)(*manifold)->GetVertexBufferPointer();
		idata = (uint32_t*)(*manifold)->GetIndexBufferPointer();
		{
			GeometryUtils::Create2MLShape(vdata2, idata, 1.5f - MANIFOLD_EPSILON, 1.0f, 1 - MANIFOLD_EPSILON, 1.2f - MANIFOLD_EPSILON, 0.6f);
		}
	});
	
	// upload to VRAM
	MetalTemporaryCommandBuffer(commandqueue, true, [&](id<MTLCommandBuffer> transfercmd) -> bool {
		box->UploadToVRAM(transfercmd);
		
		for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
			shadowcasters[i]->UploadToVRAM(transfercmd);
		}
		
		return true;
	});
	
	box->DeleteStagingBuffers();
	
	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		shadowcasters[i]->DeleteStagingBuffers();
	}
	
	// create uniform buffer
	uniformbuffer = new MetalDynamicRingBuffer(device, sizeof(CombinedUniformData), NUM_QUEUED_FRAMES, 5);
	
	// create pipelines
	MTLRenderPipelineDescriptor* pipelinedesc = [[MTLRenderPipelineDescriptor alloc] init];
	NSError* error = nil;
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_blinnphong"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_blinnphong"];
	pipelinedesc.colorAttachments[0].pixelFormat = mtkview.colorPixelFormat;
	pipelinedesc.depthAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.stencilAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.vertexDescriptor = box->GetVertexLayout();
	
	blinnphongstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!blinnphongstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_simplecolor"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_simplecolor"];
	pipelinedesc.colorAttachments[0].pixelFormat = MTLPixelFormatInvalid;
	pipelinedesc.depthAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.stencilAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.vertexDescriptor = shadowlayout;
	
	shadowstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!shadowstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.colorAttachments[0].pixelFormat = mtkview.colorPixelFormat;
	pipelinedesc.vertexDescriptor = box->GetVertexLayout();	// careful with this, simplecolor expects float4 (!!!)
	
	blackstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!blackstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.colorAttachments[0].blendingEnabled = YES;
	pipelinedesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelinedesc.vertexDescriptor = shadowlayout;
	
	yellowstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!yellowstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	// create depth-stencil states
	MTLDepthStencilDescriptor* depthdesc = [[MTLDepthStencilDescriptor alloc] init];
	
	// for black pass
	depthdesc.depthCompareFunction = MTLCompareFunctionLess;
	depthdesc.depthWriteEnabled = YES;
	
	defaultdepthstate = [device newDepthStencilStateWithDescriptor:depthdesc];
	
	// for lighting pass
	depthdesc.depthCompareFunction = MTLCompareFunctionLessEqual;
	depthdesc.depthWriteEnabled = NO;
	
	depthdesc.backFaceStencil.stencilCompareFunction = MTLCompareFunctionEqual;
	depthdesc.backFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
	depthdesc.backFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
	depthdesc.backFaceStencil.depthFailureOperation = MTLStencilOperationKeep;
	
	depthdesc.frontFaceStencil.stencilCompareFunction = MTLCompareFunctionEqual;
	depthdesc.frontFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
	depthdesc.frontFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
	depthdesc.frontFaceStencil.depthFailureOperation = MTLStencilOperationKeep;
	
	maskingstate = [device newDepthStencilStateWithDescriptor:depthdesc];
	
	// for shadow volume rendering
	depthdesc.depthCompareFunction = MTLCompareFunctionLess;
	depthdesc.depthWriteEnabled = NO;
	
	depthdesc.backFaceStencil.stencilCompareFunction = MTLCompareFunctionAlways;
	depthdesc.backFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
	depthdesc.backFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
	depthdesc.backFaceStencil.depthFailureOperation = MTLStencilOperationIncrementWrap;
	
	depthdesc.frontFaceStencil.stencilCompareFunction = MTLCompareFunctionAlways;
	depthdesc.frontFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
	depthdesc.frontFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
	depthdesc.frontFaceStencil.depthFailureOperation = MTLStencilOperationDecrementWrap;
	
	zfailstate = [device newDepthStencilStateWithDescriptor:depthdesc];

	// help text
	MTLTextureDescriptor* texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:512 height:512 mipmapped:NO];
	texdesc.usage = MTLTextureUsageShaderRead;
	
	helptext = [device newTextureWithDescriptor:texdesc];
	
	MetalRenderText("Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate light\n\n1 - Toggle silhouette\n2 - Toggle shadow volume", helptext);
	
	screenquad = new MetalScreenQuad(device, defaultlibrary, mtkview.colorPixelFormat, helptext);
	
	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(45), Math::DegreesToRadians(45), 0);

	// setup light
	light.SetDistance(10);
	light.SetPosition(0, 0, 0);
	light.SetOrientation(Math::DegreesToRadians(153), Math::DegreesToRadians(45), 0);
	light.SetPitchLimits(0.1f, 1.45f);
	
	return true;
}

void UninitScene()
{
	delete uniformbuffer;
	delete box;
	delete screenquad;

	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		delete shadowcasters[i];
	}
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		drawsilhouette = !drawsilhouette;
		break;
			
	case KeyCode2:
		drawvolume = !drawvolume;
		break;
		
	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		camera.OrbitRight(Math::DegreesToRadians(dx));
		camera.OrbitUp(Math::DegreesToRadians(dy));
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	} else if (state & MouseButtonRight) {
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz);
}

void Update(float delta)
{
	camera.Update(delta);
	light.Update(delta);
}

static void RenderScene(id<MTLRenderCommandEncoder> encoder, bool simpledraw)
{
	CombinedUniformData uniforms;
	Math::Matrix world, worldinv;
	Math::Vector3 eye;
	Math::Vector3 lightpos;
	
	camera.GetEyePosition(eye);
	light.GetEyePosition(lightpos);

	Math::MatrixScaling(world, 5, 0.1f, 5);
	Math::MatrixInverse(worldinv, world);

	CombinedUniformData* uniformdataptr = nullptr;
	uint64_t offset = uniformbuffer->MapNextRange(currentframe, (void**)&uniformdataptr);
	
	if (simpledraw)
		offset += offsetof(CombinedUniformData, simplecoloruniforms);
	
	uniforms.blinnphonguniforms.world = world;
	uniforms.blinnphonguniforms.worldinv = worldinv;
	uniforms.blinnphonguniforms.viewproj = viewproj;
	uniforms.blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
	uniforms.blinnphonguniforms.lightpos = Math::Vector4(lightpos, 1);
	uniforms.blinnphonguniforms.uvscale = Math::Vector4(3, 3, 0, 0);
	
	uniforms.simplecoloruniforms.world = world;
	uniforms.simplecoloruniforms.viewproj = viewproj;
	uniforms.simplecoloruniforms.color = Math::Vector4(0, 0, 0, 1);
	
	*uniformdataptr = uniforms;
	
	[encoder setVertexBuffer:uniformbuffer->GetBuffer() offset:offset atIndex:1];
	[encoder setFragmentTexture:wood atIndex:0];
	
	box->Draw(encoder);
	
	if (drawsilhouette)
		return;
	
	[encoder setFragmentTexture:marble atIndex:0];
	uniforms.blinnphonguniforms.uvscale = Math::Vector4(1, 1, 0, 0);
	
	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		offset = uniformbuffer->MapNextRange(currentframe, (void**)&uniformdataptr);
		
		if (simpledraw)
			offset += offsetof(CombinedUniformData, simplecoloruniforms);
		
		world = shadowcasters[i]->GetTransform();
		Math::MatrixInverse(worldinv, world);
		
		uniforms.blinnphonguniforms.world = world;
		uniforms.blinnphonguniforms.worldinv = worldinv;
		uniforms.simplecoloruniforms.world = shadowcasters[i]->GetTransform();
		
		*uniformdataptr = uniforms;
		[encoder setVertexBuffer:uniformbuffer->GetBuffer() offset:offset atIndex:1];
		
		shadowcasters[i]->GetMesh()->Draw(encoder);
	}
}

static void DrawShadowVolumes(id<MTLRenderCommandEncoder> encoder)
{
	CombinedUniformData uniforms;
	
	uniforms.simplecoloruniforms.viewproj = viewproj;
	uniforms.simplecoloruniforms.color = Math::Vector4(1, 1, 0, 0.5f);
	
	CombinedUniformData* uniformdataptr = nullptr;
	uint64_t uoffset = 0;
	uint64_t voffset = 0;
	uint64_t ioffset = 0;
	
	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		ShadowCaster* caster = shadowcasters[i];
		
		uoffset = uniformbuffer->MapNextRange(currentframe, (void**)&uniformdataptr);
		uoffset += offsetof(CombinedUniformData, simplecoloruniforms);
		
		uniforms.simplecoloruniforms.world = caster->GetTransform();
		
		*uniformdataptr = uniforms;
		[encoder setVertexBuffer:uniformbuffer->GetBuffer() offset:uoffset atIndex:1];
		
		voffset = currentframe * caster->GetVolumeVertexPitch();
		ioffset = currentframe * caster->GetVolumeIndexPitch();
		
		[encoder setVertexBuffer:caster->GetVolumeVertices() offset:voffset atIndex:0];
		[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:caster->GetNumVolumeIndices() indexType:MTLIndexTypeUInt32 indexBuffer:caster->GetVolumeIndices() indexBufferOffset:ioffset];
	}
}

static void DrawSilhouettes(id<MTLRenderCommandEncoder> encoder)
{
	CombinedUniformData uniforms;
	
	uniforms.simplecoloruniforms.viewproj = viewproj;
	uniforms.simplecoloruniforms.color = Math::Vector4(1, 1, 0, 1);
	
	CombinedUniformData* uniformdataptr = nullptr;
	uint64_t offset = 0;
	
	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		ShadowCaster* caster = shadowcasters[i];
		const auto& silhouette = caster->GetSilhouette();
		
		offset = uniformbuffer->MapNextRange(currentframe, (void**)&uniformdataptr);
		offset += offsetof(CombinedUniformData, simplecoloruniforms);
		
		uniforms.simplecoloruniforms.world = caster->GetTransform();
		
		*uniformdataptr = uniforms;
		[encoder setVertexBuffer:uniformbuffer->GetBuffer() offset:offset atIndex:1];
		
		id<MTLBuffer> vertices = [device newBufferWithLength:silhouette.size() * 2 * sizeof(Math::Vector4) options:MTLStorageModeShared|MTLResourceOptionCPUCacheModeDefault];
		
		Math::Vector4* vdata = (Math::Vector4*)[vertices contents];
		
		for (size_t j = 0; j < silhouette.size(); ++j) {
			const GeometryUtils::Edge& edge = silhouette[j];

			vdata[j * 2 + 0] = Math::Vector4(edge.v1.x, edge.v1.y, edge.v1.z, 1);
			vdata[j * 2 + 1] = Math::Vector4(edge.v2.x, edge.v2.y, edge.v2.z, 1);
		}
		
		[encoder setVertexBuffer:vertices offset:0 atIndex:0];
		[encoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:silhouette.size() * 2];
		
		vertices = nil;
	}
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix world, view, proj;
	Math::Vector4 lightpos(0, 0, 0, 1);
	
	// will wait until a command buffer finished execution
	dispatch_semaphore_wait(flightsema, DISPATCH_TIME_FOREVER);
	
	__block dispatch_semaphore_t blocksema = flightsema;
	id<MTLCommandBuffer> commandbuffer = [commandqueue commandBuffer];
	
	[commandbuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
		dispatch_semaphore_signal(blocksema);
	}];
	
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	
	light.Animate(alpha);
	light.GetEyePosition((Math::Vector3&)lightpos);
	
	// put far plane to infinity
	proj._33 = -1;
	proj._43 = -camera.GetNearPlane();
	
	Math::MatrixMultiply(viewproj, view, proj);
	
	// calculate shadow volumes
	for (int i = 0; i < ARRAY_SIZE(shadowcasters); ++i) {
		shadowcasters[i]->ExtrudeSilhouette(currentframe, lightpos);
	}
	
	// render
	MTLRenderPassDescriptor* renderpassdesc = mtkview.currentRenderPassDescriptor;
	id<MTLTexture> oldtarget = renderpassdesc.colorAttachments[0].texture;
	
	if (renderpassdesc) {
		// black pass
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionStore;
		renderpassdesc.stencilAttachment.storeAction = MTLStoreActionStore;
		
		id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			
			[encoder setRenderPipelineState:blackstate];
			[encoder setDepthStencilState:defaultdepthstate];
			
			RenderScene(encoder, true);
			
		}
		[encoder endEncoding];
		
		// fill stencil buffer
		renderpassdesc.colorAttachments[0].texture = nil;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeNone];
			
			[encoder setRenderPipelineState:shadowstate];
			[encoder setDepthStencilState:zfailstate];
			
			DrawShadowVolumes(encoder);
		}
		[encoder endEncoding];
		
		// lighting pass
		renderpassdesc.colorAttachments[0].texture = oldtarget;
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			
			[encoder setRenderPipelineState:blinnphongstate];
			[encoder setDepthStencilState:maskingstate];
			[encoder setStencilReferenceValue:0];
			
			RenderScene(encoder, false);
			
			if (drawsilhouette) {
				[encoder setRenderPipelineState:yellowstate];
				[encoder setDepthStencilState:defaultdepthstate];
				
				DrawSilhouettes(encoder);
			}
			
			if (drawvolume) {
				[encoder setRenderPipelineState:yellowstate];
				[encoder setDepthStencilState:defaultdepthstate];
				
				DrawShadowVolumes(encoder);
			}
		}
		[encoder endEncoding];
		
		// render text
		MTLViewport viewport;
		auto depthattachment = renderpassdesc.depthAttachment;
		auto stencilattachment = renderpassdesc.stencilAttachment;

		renderpassdesc.depthAttachment = nil;
		renderpassdesc.stencilAttachment = nil;
		
		viewport.originX	= 10;
		viewport.originY	= 10;
		viewport.width		= 512;
		viewport.height		= 512;
		viewport.znear		= 0;
		viewport.zfar		= 1;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeBack];
			[encoder setViewport:viewport];
			
			screenquad->Draw(encoder);
		}
		[encoder endEncoding];
		
		renderpassdesc.depthAttachment = depthattachment;
		renderpassdesc.stencilAttachment = stencilattachment;
		
		[commandbuffer presentDrawable:mtkview.currentDrawable];
		currentframe = (currentframe + 1) % NUM_QUEUED_FRAMES;
	}

	[commandbuffer commit];
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);
	
	if (!app->InitializeDriverInterface(GraphicsAPIMetal)) {
		delete app;
		return 1;
	}
	
	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseScrollCallback = MouseScroll;
	
	app->Run();
	delete app;
	
	return 0;
}
