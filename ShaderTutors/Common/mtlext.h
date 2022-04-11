
#ifndef _MTLEXT_H_
#define _MTLEXT_H_

#import <Metal/Metal.h>
#import <functional>
#import <map>
#import <string>
#import <cstdint>
#import <cassert>

#include "3Dmath.h"

#ifdef DEBUG
#	define MTL_ASSERT(x)	assert(x)
#else
#	define MTL_ASSERT(x)	{ if (!(x)) { throw 1; } }
#endif

// --- Enums ------------------------------------------------------------------

enum MetalMeshFlags {
	MTL_MESH_32BIT = 1,
	MTL_MESH_SYSTEMMEM = 2
};

// --- Structures -------------------------------------------------------------

struct MetalAttributeRange
{
	MTLPrimitiveType	primitiveType;
	uint32_t			attribId;
	uint32_t			indexStart;
	uint32_t			indexCount;
	uint32_t			vertexStart;
	uint32_t			vertexCount;
	bool				enabled;
};

struct MetalMaterial
{
	Math::Color			diffuse;
	Math::Color			ambient;
	Math::Color			specular;
	Math::Color			emissive;
	float				power;
	id<MTLTexture>		texture;
	id<MTLTexture>		normalMap;
	
	MetalMaterial();
	~MetalMaterial();
};

/**
 * \brief Don't load content items more than once.
 */
class MetalContentRegistry
{
	typedef std::map<std::string, id<MTLTexture> > TextureMap;
	
private:
	static MetalContentRegistry* _inst;
	
	TextureMap textures;
	
	MetalContentRegistry();
	~MetalContentRegistry();
	
public:
	static MetalContentRegistry& Instance();
	static void Release();
	
	void RegisterTexture(const std::string& file, id<MTLTexture> tex);
	void UnregisterTexture(id<MTLTexture> tex);
	
	id<MTLTexture> PointerTexture(const std::string& file);
};

inline MetalContentRegistry& MetalContentManager() {
	return MetalContentRegistry::Instance();
}

/**
 * \brief Triangle list mesh.
 */
class MetalMesh
{
private:
	Math::AABox				boundingbox;
	
	id<MTLBuffer>			vertexbuffer;
	id<MTLBuffer> 			indexbuffer;
	id<MTLBuffer>			stagingvertexbuffer;
	id<MTLBuffer>			stagingindexbuffer;
	
	MTLVertexDescriptor*	vertexlayout;
	MetalAttributeRange*	subsettable;
	MetalMaterial*			materials;
	
	uint8_t*				mappedvdata;
	uint8_t*				mappedidata;
	
	uint64_t				baseoffset;
	uint64_t				indexoffset;
	uint64_t				totalsize;
	uint32_t				vstride;
	uint32_t				vertexcount;
	uint32_t				indexcount;
	uint32_t				numsubsets;
	MTLIndexType			indexformat;
	bool					inherited;

public:
	static MetalMesh* LoadFromQM(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, id<MTLBuffer> buffer = nil, uint64_t offset = 0);
	
	MetalMesh(id<MTLDevice> device, uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, id<MTLBuffer> buffer = nil, uint64_t offset = 0, uint32_t flags = 0);
	MetalMesh(id<MTLDevice> device, uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, MTLVertexDescriptor* layout, uint32_t flags = 0);
	~MetalMesh();
	
	void Draw(id<MTLRenderCommandEncoder> encoder);
	void DrawSubset(id<MTLRenderCommandEncoder> encoder, uint32_t index);
	void EnableSubset(uint32_t index, bool enable);
	void UploadToVRAM(id<MTLCommandBuffer> commandbuffer);
	void DeleteStagingBuffers();
	void GenerateTangentFrame(id<MTLDevice> device);
	
	void* GetVertexBufferPointer();
	void* GetIndexBufferPointer();
	
	MTLVertexDescriptor* GetVertexLayout();
	MTLVertexDescriptor* GetVertexLayout(NSArray* signature);
	
	inline uint32_t GetVertexStride() const					{ return vstride; }
	inline uint32_t GetIndexStride() const					{ return (indexformat == MTLIndexTypeUInt16 ? 2 : 4); }
	inline uint32_t GetNumVertices() const					{ return vertexcount; }
	inline uint32_t GetNumPolygons() const					{ return indexcount / 3; }
	inline uint32_t GetNumSubsets() const					{ return numsubsets; }
	
	inline MetalMaterial& GetMaterial(uint32_t subset) 		{ return materials[subset]; }
	inline const Math::AABox& GetBoundingBox() const		{ return boundingbox; }
};

/**
 * \brief Makes 2D rendering easier.
 */
class MetalScreenQuad
{
private:
	Math::Matrix transform;
	id<MTLRenderPipelineState> pipelinestate;
	id<MTLDepthStencilState> depthstate;
	id<MTLTexture> texture;

public:
	MetalScreenQuad(id<MTLDevice> device, id<MTLLibrary> library, MTLPixelFormat attachmentformat, id<MTLTexture> tex, NSString* psfunction = @"ps_screenquad");
	~MetalScreenQuad();
	
	void Draw(id<MTLRenderCommandEncoder> encoder);
	void SetTextureMatrix(const Math::Matrix& m);
};

/**
 * \brief Helps with small data management.
 */
class MetalDynamicRingBuffer
{
	typedef void (^RestartHandler)();
	
private:
	id<MTLDevice>	mtldevice;
	id<MTLBuffer>	buffer;
	uint64_t		totalsize;
	uint64_t		offset;
	uint32_t		changecount;	// expected changes per flight
	uint32_t		stride;
	uint8_t			flightcount;
	
public:
	MetalDynamicRingBuffer(id<MTLDevice> device, uint32_t datastride, uint8_t numflights, uint32_t numchanges);
	~MetalDynamicRingBuffer();
	
	uint64_t MapNextRange(uint8_t flight, void** result);
	
	inline id<MTLBuffer> GetBuffer()	{ return buffer; }
};

// --- Functions --------------------------------------------------------------

void MetalRenderText(const std::string& str, id<MTLTexture> texture);
void MetalRenderTextEx(const std::string& str, id<MTLTexture> texture, const char* face, bool border, int style, float emsize);
void MetalTemporaryCommandBuffer(id<MTLCommandQueue> queue, bool wait, std::function<bool (id<MTLCommandBuffer>)> callback);

id<MTLTexture> MetalCreateTextureFromFile(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, bool srgb);
id<MTLTexture> MetalCreateTextureFromDDS(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, bool srgb);

#endif
