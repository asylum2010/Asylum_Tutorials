
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>

#include "mtlext.h"
#include "geometryutils.h"
#include "dds.h"

static void ReadString(FILE* f, char* buff)
{
	size_t ind = 0;
	char ch = fgetc(f);
	
	while (ch != '\n') {
		buff[ind] = ch;
		ch = fgetc(f);
		
		++ind;
	}
	
	buff[ind] = '\0';
}

static std::pair<MTLVertexFormat, uint32_t> MapDataType(MTLDataType type)
{
	switch (type) {
		case MTLDataTypeFloat:
			return { MTLVertexFormatFloat, 4 };
			
		case MTLDataTypeFloat2:
			return { MTLVertexFormatFloat2, 8 };
			
		case MTLDataTypeFloat3:
			return { MTLVertexFormatFloat3, 12 };
			
		case MTLDataTypeFloat4:
			return { MTLVertexFormatFloat4, 16 };
			
		case MTLDataTypeHalf4:
			return { MTLVertexFormatHalf4, 8 };
			
		case MTLDataTypeUChar4:
			return { MTLVertexFormatUChar4, 4 };
			
		default:
			break;
	}
	
	return { MTLVertexFormatFloat4, 16 };
}

// --- MetalMaterial impl -----------------------------------------------------

MetalMaterial::MetalMaterial()
{
	texture = nil;
	normalMap = nil;
}

MetalMaterial::~MetalMaterial()
{
	texture = nil;
	normalMap = nil;
}

// --- MetalContentRegistry impl ----------------------------------------------

MetalContentRegistry* MetalContentRegistry::_inst = nullptr;

MetalContentRegistry& MetalContentRegistry::Instance()
{
	if (_inst == nullptr)
		_inst = new MetalContentRegistry();
	
	return *_inst;
}

void MetalContentRegistry::Release()
{
	if (_inst != nullptr)
		delete _inst;
	
	_inst = nullptr;
}

MetalContentRegistry::MetalContentRegistry()
{
}

MetalContentRegistry::~MetalContentRegistry()
{
}

void MetalContentRegistry::RegisterTexture(const std::string& file, id<MTLTexture> tex)
{
	std::string name;
	Math::GetFile(name, file);
	
	MTL_ASSERT(textures.count(name) == 0);
	textures.insert(TextureMap::value_type(name, tex));
}

void MetalContentRegistry::UnregisterTexture(id<MTLTexture> tex)
{
	for (TextureMap::iterator it = textures.begin(); it != textures.end(); ++it) {
		if (it->second == tex) {
			textures.erase(it);
			break;
		}
	}
}

id<MTLTexture> MetalContentRegistry::PointerTexture(const std::string& file)
{
	std::string name;
	Math::GetFile(name, file);
	
	TextureMap::iterator it = textures.find(name);
	
	if (it == textures.end())
		return nil;

	return it->second;
}

// --- MetalMesh impl ---------------------------------------------------------

MetalMesh::MetalMesh(id<MTLDevice> device, uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, id<MTLBuffer> buffer, uint64_t offset, uint32_t flags)
{
	totalsize		= numvertices * vertexstride;
	vertexcount		= numvertices;
	indexcount		= numindices;
	vstride			= vertexstride;
	baseoffset		= ((buffer != nil) ? offset : 0);
	inherited		= (buffer != nil);
	numsubsets		= 1;
	mappedvdata		= nullptr;
	mappedidata		= nullptr;
	vertexlayout	= [[MTLVertexDescriptor alloc] init];

	if (numvertices > 0xffff || (flags & MTL_MESH_32BIT))
		indexformat = MTLIndexTypeUInt32;
	else
		indexformat = MTLIndexTypeUInt16;
	
	uint32_t istride = ((indexformat == MTLIndexTypeUInt16) ? 2 : 4);
	
	if (buffer != nil) {
		if (totalsize % 4 > 0)
			totalsize += (4 - (totalsize % 4));
		
		indexoffset = baseoffset + totalsize;
		totalsize += numindices * istride;
		
		if (totalsize % 256 > 0)
			totalsize += (256 - (totalsize % 256));

		vertexbuffer = indexbuffer = buffer;
		MTL_ASSERT(baseoffset + totalsize <= [buffer length]);
		
		mappedvdata = mappedidata = (uint8_t*)[buffer contents];
	} else {
		totalsize += numindices * istride;

		vertexbuffer = [device newBufferWithLength:numvertices * vertexstride options:MTLResourceStorageModePrivate];
		indexbuffer = [device newBufferWithLength:numindices * istride options:MTLResourceStorageModePrivate];

		stagingvertexbuffer = [device newBufferWithLength:numvertices * vertexstride options:MTLResourceStorageModeShared|MTLResourceCPUCacheModeDefaultCache];
		stagingindexbuffer = [device newBufferWithLength:numindices * istride options:MTLResourceStorageModeShared|MTLResourceCPUCacheModeDefaultCache];
		
		mappedvdata = (uint8_t*)[stagingvertexbuffer contents];
		mappedidata = (uint8_t*)[stagingindexbuffer contents];
		
		indexoffset = 0;
	}
	
	// use common vertex layout
	vertexlayout.attributes[0].bufferIndex = 0;
	vertexlayout.attributes[0].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[0].offset = 0;
	
	vertexlayout.attributes[1].bufferIndex = 0;
	vertexlayout.attributes[1].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[1].offset = 12;
	
	vertexlayout.attributes[2].bufferIndex = 0;
	vertexlayout.attributes[2].format = MTLVertexFormatFloat2;
	vertexlayout.attributes[2].offset = 24;
	
	static_assert(sizeof(GeometryUtils::CommonVertex) == 32, "sizeof(CommonVertex) should be 32 bytes");
	
	vertexlayout.layouts[0].stride = sizeof(GeometryUtils::CommonVertex);
	vertexlayout.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	
	subsettable = new MetalAttributeRange[1];
	materials = new MetalMaterial[1];
	
	subsettable[0].attribId = 0;
	subsettable[0].enabled = true;
	subsettable[0].indexCount = numindices;
	subsettable[0].indexStart = 0;
	subsettable[0].primitiveType = MTLPrimitiveTypeTriangle;
	subsettable[0].vertexCount = numvertices;
	subsettable[0].vertexStart = 0;
}

MetalMesh::MetalMesh(id<MTLDevice> device, uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, MTLVertexDescriptor* layout, uint32_t flags)
{
	totalsize		= numvertices * vertexstride;
	vertexcount		= numvertices;
	indexcount		= numindices;
	vstride			= vertexstride;
	baseoffset		= 0;
	inherited		= false;
	numsubsets		= 1;
	mappedvdata		= nullptr;
	mappedidata		= nullptr;
	vertexlayout	= layout;
	
	if (numvertices > 0xffff || (flags & MTL_MESH_32BIT))
		indexformat = MTLIndexTypeUInt32;
	else
		indexformat = MTLIndexTypeUInt16;
	
	uint32_t istride = ((indexformat == MTLIndexTypeUInt16) ? 2 : 4);
	
	totalsize += numindices * istride;

	if ((flags & MTL_MESH_SYSTEMMEM) == 0) {
		vertexbuffer = [device newBufferWithLength:numvertices * vertexstride options:MTLResourceStorageModePrivate];
		indexbuffer = [device newBufferWithLength:numindices * istride options:MTLResourceStorageModePrivate];
	} else {
		// system memory only
		vertexbuffer = nil;
		indexbuffer = nil;
	}
	
	stagingvertexbuffer = [device newBufferWithLength:numvertices * vertexstride options:MTLResourceStorageModeShared|MTLResourceCPUCacheModeDefaultCache];
	stagingindexbuffer = [device newBufferWithLength:numindices * istride options:MTLResourceStorageModeShared|MTLResourceCPUCacheModeDefaultCache];
	
	mappedvdata = (uint8_t*)[stagingvertexbuffer contents];
	mappedidata = (uint8_t*)[stagingindexbuffer contents];
	
	indexoffset = 0;
	
	subsettable = new MetalAttributeRange[1];
	materials = new MetalMaterial[1];
	
	subsettable[0].attribId = 0;
	subsettable[0].enabled = true;
	subsettable[0].indexCount = numindices;
	subsettable[0].indexStart = 0;
	subsettable[0].primitiveType = MTLPrimitiveTypeTriangle;
	subsettable[0].vertexCount = numvertices;
	subsettable[0].vertexStart = 0;
}

MetalMesh::~MetalMesh()
{
	if (subsettable != nullptr)
		delete[] subsettable;
	
	if (materials != nullptr)
		delete[] materials;
}

void MetalMesh::Draw(id<MTLRenderCommandEncoder> encoder)
{
	for (uint32_t i = 0; i < numsubsets; ++i)
		DrawSubset(encoder, i);
}

void MetalMesh::DrawSubset(id<MTLRenderCommandEncoder> encoder, uint32_t index)
{
	MTL_ASSERT(index < numsubsets);
	
	const MetalAttributeRange& subset = subsettable[index];
	const MetalMaterial& material = materials[index];
	
	if (subset.enabled) {
		if (material.texture != nil)
			[encoder setFragmentTexture:material.texture atIndex:0];
		
		if (material.normalMap != nil)
			[encoder setFragmentTexture:material.normalMap atIndex:1];
		
		uint32_t istride = (indexformat == MTLIndexTypeUInt16 ? 2 : 4);
		
		[encoder setVertexBuffer:vertexbuffer offset:baseoffset atIndex:0];
		[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:subset.indexCount indexType:indexformat indexBuffer:indexbuffer indexBufferOffset:indexoffset + subset.indexStart * istride];
	}
}

void MetalMesh::EnableSubset(uint32_t index, bool enable)
{
	MTL_ASSERT(index < numsubsets);
	subsettable[index].enabled = enable;
}

void MetalMesh::GenerateTangentFrame(id<MTLDevice> device)
{
	MTL_ASSERT(vstride == sizeof(GeometryUtils::CommonVertex));
	MTL_ASSERT(stagingvertexbuffer != nullptr);
	MTL_ASSERT(!inherited);
	
	id<MTLBuffer>					newbuffer	= [device newBufferWithLength:vertexcount * sizeof(GeometryUtils::TBNVertex) options:MTLResourceStorageModeShared|MTLResourceCPUCacheModeDefaultCache];
	GeometryUtils::CommonVertex*	oldvdata	= (GeometryUtils::CommonVertex*)[stagingvertexbuffer contents];
	GeometryUtils::TBNVertex*		newvdata	= (GeometryUtils::TBNVertex*)[newbuffer contents];
	void*							idata		= [stagingindexbuffer contents];
	uint32_t						i1, i2, i3;
	bool							is32bit		= (indexformat == MTLIndexTypeUInt32);
	
	for (uint32_t i = 0; i < numsubsets; ++i) {
		const MetalAttributeRange& subset = subsettable[i];
		MTL_ASSERT(subset.indexCount > 0);
		
		GeometryUtils::CommonVertex*	oldsubsetdata	= (oldvdata + subset.vertexStart);
		GeometryUtils::TBNVertex*		newsubsetdata	= (newvdata + subset.vertexStart);
		void*							subsetidata		= ((uint16_t*)idata + subset.indexStart);
		
		if (is32bit)
			subsetidata = ((uint32_t*)idata + subset.indexStart);
		
		// initialize new data
		for (uint32_t j = 0; j < subset.vertexCount; ++j) {
			GeometryUtils::CommonVertex& oldvert = oldsubsetdata[j];
			GeometryUtils::TBNVertex& newvert = newsubsetdata[j];
			
			newvert.x = oldvert.x;
			newvert.y = oldvert.y;
			newvert.z = oldvert.z;
			
			newvert.nx = oldvert.nx;
			newvert.ny = oldvert.ny;
			newvert.nz = oldvert.nz;
			
			newvert.u = oldvert.u;
			newvert.v = oldvert.v;
			
			newvert.tx = newvert.bx = 0;
			newvert.ty = newvert.by = 0;
			newvert.tz = newvert.bz = 0;
		}
		
		for (uint32_t j = 0; j < subset.indexCount; j += 3) {
			if (is32bit) {
				i1 = *((uint32_t*)subsetidata + j + 0) - subset.vertexStart;
				i2 = *((uint32_t*)subsetidata + j + 1) - subset.vertexStart;
				i3 = *((uint32_t*)subsetidata + j + 2) - subset.vertexStart;
			} else {
				i1 = *((uint16_t*)subsetidata + j + 0) - subset.vertexStart;
				i2 = *((uint16_t*)subsetidata + j + 1) - subset.vertexStart;
				i3 = *((uint16_t*)subsetidata + j + 2) - subset.vertexStart;
			}
			
			GeometryUtils::AccumulateTangentFrame(newsubsetdata, i1, i2, i3);
		}
		
		for (uint32_t j = 0; j < subset.vertexCount; ++j) {
			GeometryUtils::OrthogonalizeTangentFrame(newsubsetdata[j]);
		}
	}

	stagingvertexbuffer = newbuffer;
	vstride = sizeof(GeometryUtils::TBNVertex);
	
	vertexbuffer = [device newBufferWithLength:vertexcount * vstride options:MTLResourceStorageModePrivate];
	
	vertexlayout.attributes[0].bufferIndex = 0;
	vertexlayout.attributes[0].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[0].offset = 0;
	
	vertexlayout.attributes[1].bufferIndex = 0;
	vertexlayout.attributes[1].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[1].offset = 12;
	
	vertexlayout.attributes[2].bufferIndex = 0;
	vertexlayout.attributes[2].format = MTLVertexFormatFloat2;
	vertexlayout.attributes[2].offset = 24;
	
	vertexlayout.attributes[3].bufferIndex = 0;
	vertexlayout.attributes[3].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[3].offset = 32;
	
	vertexlayout.attributes[4].bufferIndex = 0;
	vertexlayout.attributes[4].format = MTLVertexFormatFloat3;
	vertexlayout.attributes[4].offset = 44;
	
	vertexlayout.layouts[0].stride = vstride;
	vertexlayout.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
}

void* MetalMesh::GetVertexBufferPointer()
{
	return (mappedvdata + baseoffset);
}

void* MetalMesh::GetIndexBufferPointer()
{
	return (mappedidata + indexoffset);
}

MTLVertexDescriptor* MetalMesh::GetVertexLayout()
{
	return vertexlayout;
}

MTLVertexDescriptor* MetalMesh::GetVertexLayout(NSArray* signature)
{
	if (signature == nil)
		return nil;
	
	MTLVertexDescriptor* newdescriptor = [[MTLVertexDescriptor alloc] init];
	NSUInteger offset = 0;
	
	for (int i = 0; i < [signature count]; ++i) {
		MTLVertexAttribute* attrib = [signature objectAtIndex:i];
		auto formatandsize = MapDataType(attrib.attributeType);

		newdescriptor.attributes[i].bufferIndex = 0;
		newdescriptor.attributes[i].format = formatandsize.first;
		newdescriptor.attributes[i].offset = offset;
		
		offset += formatandsize.second;
		
		// must be sequential
		MTL_ASSERT([attrib attributeIndex] == i);
	}
	
	// NOTE: stride must be preserved!
	newdescriptor.layouts[0].stride = vertexlayout.layouts[0].stride;
	newdescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	
	return newdescriptor;
}

void MetalMesh::UploadToVRAM(id<MTLCommandBuffer> commandbuffer)
{
	if (vertexbuffer == nil || indexbuffer == nil)
		return;
	
	if (!inherited && (mappedvdata != nullptr || mappedidata != nullptr)) {
		mappedvdata = mappedidata = nullptr;
	}
	
	id<MTLBlitCommandEncoder> encoder = [commandbuffer blitCommandEncoder];
	
	if (inherited) {
		// do nothing, the caller will synchronize
	} else {
		[encoder copyFromBuffer:stagingvertexbuffer sourceOffset:0 toBuffer:vertexbuffer destinationOffset:0 size:stagingvertexbuffer.length];
		[encoder copyFromBuffer:stagingindexbuffer sourceOffset:0 toBuffer:indexbuffer destinationOffset:0 size:stagingindexbuffer.length];
	}
	
	[encoder endEncoding];
}

void MetalMesh::DeleteStagingBuffers()
{
	stagingvertexbuffer = nil;
	stagingindexbuffer = nil;
}

MetalMesh* MetalMesh::LoadFromQM(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, id<MTLBuffer> buffer, uint64_t offset)
{
	static const uint16_t elemsizes[] = {
		1,	// float
		2,	// float2
		3,	// float3
		4,	// float4
		4,	// color
		4	// ubyte4
	};
	
	static const uint16_t elemstrides[] = {
		4,	// float
		4,	// float2
		4,	// float3
		4,	// float4
		1,	// color
		1	// ubyte4
	};
	
	FILE* infile = fopen(file, "rb");
	
	if (!infile)
		return nullptr;
	
	Math::Vector3	bbmin, bbmax;
	char			buff[256];
	std::string		basedir(file), str;
	
	uint32_t 		unused;
	uint32_t 		numvertices;
	uint32_t 		numindices;
	uint32_t 		istride;
	uint32_t 		vstride = 0;
	uint32_t 		numsubsets;
	uint32_t 		version;
	uint8_t 		elemtype;
	
	basedir = basedir.substr(0, basedir.find_last_of('/') + 1);
	
	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);
	
	version = unused >> 16;
	
	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	
	// vertex layout
	fread(&unused, 4, 1, infile);
	
	for (uint32_t i = 0; i < unused; ++i) {
		fseek(infile, 3, SEEK_CUR);
		fread(&elemtype, 1, 1, infile);
		fseek(infile, 1, SEEK_CUR);
		
		vstride += elemsizes[elemtype] * elemstrides[elemtype];
	}
	
	MetalMesh* mesh = new MetalMesh(device, numvertices, numindices, vstride);
	
	void* vdata = mesh->GetVertexBufferPointer();
	fread(vdata, vstride, numvertices, infile);
	
	void* idata = mesh->GetIndexBufferPointer();
	fread(idata, istride, numindices, infile);
	
	delete[] mesh->subsettable;
	delete[] mesh->materials;
	
	// subset and material info
	mesh->numsubsets = numsubsets;
	mesh->subsettable = new MetalAttributeRange[numsubsets];
	mesh->materials = new MetalMaterial[numsubsets];
	
	if (version >= 1) {
		fread(&unused, 4, 1, infile);
		
		if (unused > 0)
			fseek(infile, 8 * unused, SEEK_CUR);
	}
	
	for (uint32_t i = 0; i < numsubsets; ++i) {
		MetalAttributeRange& subset = mesh->subsettable[i];
		MetalMaterial& material = mesh->materials[i];
		
		subset.attribId = i;
		subset.primitiveType = MTLPrimitiveTypeTriangle;
		subset.enabled = true;
		
		fread(&subset.indexStart, 4, 1, infile);
		fread(&subset.vertexStart, 4, 1, infile);
		fread(&subset.vertexCount, 4, 1, infile);
		fread(&subset.indexCount, 4, 1, infile);
		
		fread(&bbmin, sizeof(float), 3, infile);
		fread(&bbmax, sizeof(float), 3, infile);
		
		mesh->boundingbox.Add(bbmin);
		mesh->boundingbox.Add(bbmax);
		
		// subset & material info
		ReadString(infile, buff);
		ReadString(infile, buff);
		
		if (buff[1] != ',') {
			fread(&material.ambient, sizeof(Math::Color), 1, infile);
			fread(&material.diffuse, sizeof(Math::Color), 1, infile);
			fread(&material.specular, sizeof(Math::Color), 1, infile);
			fread(&material.emissive, sizeof(Math::Color), 1, infile);
			
			if (version >= 2)
				fseek(infile, 16, SEEK_CUR);	// uvscale
			
			fread(&material.power, sizeof(float), 1, infile);
			fread(&material.diffuse.a, sizeof(float), 1, infile);
			fread(&unused, 4, 1, infile);		// blend mode
			
			ReadString(infile, buff);
			
			if (buff[1] != ',') {
				str = basedir + buff;
				material.texture = MetalCreateTextureFromFile(device, queue, str.c_str(), true);
			}
			
			ReadString(infile, buff);
			
			if (buff[1] != ',') {
				str = basedir + buff;
				material.normalMap = MetalCreateTextureFromFile(device, queue, str.c_str(), false);
			}
			
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
		} else {
			material.ambient	= Math::Color(0, 0, 0, 1);
			material.diffuse	= Math::Color(1, 1, 1, 1);
			material.specular	= Math::Color(1, 1, 1, 1);
			material.emissive	= Math::Color(0, 0, 0, 1);
			material.power		= 80;
			material.texture	= nullptr;
			material.normalMap	= nullptr;
		}
		
		// texture info
		ReadString(infile, buff);
		
		if (buff[1] != ',' && material.texture == nullptr) {
			str = basedir + buff;
			material.texture = MetalCreateTextureFromFile(device, queue, str.c_str(), true);
		}
		
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
	}
	
	fclose(infile);
	
	return mesh;
}

// --- MetalScreenQuad impl ---------------------------------------------------

MetalScreenQuad::MetalScreenQuad(id<MTLDevice> device, id<MTLLibrary> library, MTLPixelFormat attachmentformat, id<MTLTexture> tex, NSString* psfunction)
{
	MTLRenderPipelineDescriptor* pipelinedesc = [[MTLRenderPipelineDescriptor alloc] init];
	MTLDepthStencilDescriptor* depthdesc = [[MTLDepthStencilDescriptor alloc] init];
	NSError* error = nil;
	
	texture = tex;
	
	pipelinedesc.colorAttachments[0].pixelFormat = attachmentformat;
	pipelinedesc.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
	pipelinedesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
	
	pipelinedesc.vertexFunction = [library newFunctionWithName:@"vs_screenquad"];
	pipelinedesc.fragmentFunction = [library newFunctionWithName:psfunction];
	
	pipelinedesc.colorAttachments[0].blendingEnabled = YES;
	pipelinedesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelinedesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	
	pipelinestate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!pipelinestate) {
		NSLog(@"Error: %@", error);
	}
	
	depthdesc.depthCompareFunction = MTLCompareFunctionAlways;
	depthdesc.depthWriteEnabled = NO;
	
	depthstate = [device newDepthStencilStateWithDescriptor:depthdesc];
}

MetalScreenQuad::~MetalScreenQuad()
{
}

void MetalScreenQuad::Draw(id<MTLRenderCommandEncoder> encoder)
{
	[encoder setRenderPipelineState:pipelinestate];
	[encoder setDepthStencilState:depthstate];
	//[encoder setVertexBytes:&transform._11 length:sizeof(Math::Matrix) atIndex:0];
	[encoder setFragmentTexture:texture atIndex:0];
	
	[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:1];
}

void MetalScreenQuad::SetTextureMatrix(const Math::Matrix& m)
{
	transform = m;
}

// --- MetalDynamicRingBuffer impl --------------------------------------------

MetalDynamicRingBuffer::MetalDynamicRingBuffer(id<MTLDevice> device, uint32_t datastride, uint8_t numflights, uint32_t numchanges)
{
	totalsize = datastride * numflights * numchanges;
	buffer = [device newBufferWithLength:totalsize options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	
	mtldevice		= device;
	stride			= datastride;
	changecount		= numchanges;
	offset			= 0;
	flightcount		= numflights;
}

MetalDynamicRingBuffer::~MetalDynamicRingBuffer()
{
	mtldevice = nil;
	buffer = nil;
}

uint64_t MetalDynamicRingBuffer::MapNextRange(uint8_t flight, void** result)
{
	// TODO: find a way to detect when no orphaning is needed
	(void)flight;
	
	if (offset + stride > totalsize) {
		// just orphan the buffer (driver will drop it when finishes)
		buffer = [mtldevice newBufferWithLength:totalsize options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
		offset = stride;
	} else {
		offset += stride;
	}
	
	uint8_t* bytedata = (uint8_t*)[buffer contents];
	(*result) = bytedata + (offset - stride);
	
	return offset - stride;
}

// --- Functions impl ---------------------------------------------------------

void MetalRenderText(const std::string& str, id<MTLTexture> texture)
{
	MetalRenderTextEx(str, texture, "Arial", true, kCTFontTraitBold, 25.0f);
}

void MetalRenderTextEx(const std::string& str, id<MTLTexture> texture, const char* face, bool border, int style, float emsize)
{
	CGColorSpaceRef	colorspace		= CGColorSpaceCreateDeviceRGB();
	CGContextRef 	cgcontext 		= CGBitmapContextCreate(NULL, texture.width, texture.height, 8, texture.width * 4, colorspace, kCGImageAlphaNoneSkipLast);
	
	CFStringRef 	font_name 		= CFStringCreateWithCString(NULL, face, kCFStringEncodingMacRoman);
	CTFontRef 		basefont		= CTFontCreateWithName(font_name, emsize, NULL);
	CTFontRef		font			= CTFontCreateCopyWithSymbolicTraits(basefont, emsize, NULL, style, style);
	CGColorRef 		strokecolor		= CGColorCreateGenericRGB(0, 0, 0, 1);
	CGColorRef 		fillcolor 		= CGColorCreateGenericRGB(1, 1, 1, 1);
	
	CFStringRef 	keys[] 			= { kCTFontAttributeName, kCTForegroundColorAttributeName };
	CFTypeRef 		values[] 		= { font, fillcolor };
	CFDictionaryRef	attributes		= CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys, (const void**)&values, sizeof(keys) / sizeof(keys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	
	CFRelease(font_name);
	CFRelease(font);
	CFRelease(basefont);
	CGColorRelease(strokecolor);
	CGColorRelease(fillcolor);
	
	CGRect					rect		= CGRectMake(0, 0, texture.width, texture.height);
	CGMutablePathRef		path		= CGPathCreateMutable();
	
	CGPathAddRect(path, NULL, rect);
	
	// draw text to CG context
	CFStringRef				cfstr		= CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)str.c_str (), str.length() * sizeof(char), kCFStringEncodingUTF8, false);
	CFAttributedStringRef	attrstr		= CFAttributedStringCreate(kCFAllocatorDefault, cfstr, attributes);
	CTFramesetterRef		framesetter	= CTFramesetterCreateWithAttributedString((CFAttributedStringRef)attrstr);
	CTFrameRef				frame		= CTFramesetterCreateFrame(framesetter, CFRangeMake(0, CFAttributedStringGetLength(attrstr)), path, NULL);

	//CGContextSetStrokeColorWithColor(cgcontext, stroke);
	//CGContextSetFillColorWithColor(cgcontext, stroke);
	//CGContextSetTextDrawingMode(cgcontext, kCGTextFillStroke);
	
	CGContextSetTextPosition(cgcontext, 0.0f, texture.height - emsize);
	CTFrameDraw(frame, cgcontext);
	
	// copy to texture
	MTLRegion region;
	void* data = CGBitmapContextGetData(cgcontext);
	size_t bytesperrow = CGBitmapContextGetBytesPerRow(cgcontext);
	
	region.origin.x = region.origin.y = region.origin.z = 0;
	
	region.size.width = texture.width;
	region.size.height = texture.height;
	region.size.depth = 1;
	
	[texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesperrow];

	// clean up
	CFRelease(frame);
	CFRelease(path);
	CFRelease(framesetter);
	CFRelease(cfstr);

	CGColorSpaceRelease(colorspace);
	CGContextRelease(cgcontext);
}

void MetalTemporaryCommandBuffer(id<MTLCommandQueue> queue, bool wait, std::function<bool (id<MTLCommandBuffer>)> callback)
{
	id<MTLCommandBuffer> cmdbuff = [queue commandBuffer];
	
	if (callback != nullptr)
		callback(cmdbuff);
	
	[cmdbuff commit];
	
	if (wait) {
		[cmdbuff waitUntilCompleted];
	}
}

id<MTLTexture> MetalCreateTextureFromFile(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, bool srgb)
{
	std::string name;
	Math::GetFile(name, file);
	
	id<MTLTexture> ret = MetalContentManager().PointerTexture(name);
	
	if (ret != nil) {
		printf("Pointer %s\n", name.c_str());
		return ret;
	}
	
	NSURL* path = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file]];
	NSImage* img = [[NSImage alloc] initByReferencingURL:path];
	
	NSSize size = img.size;
	NSRect rect = NSMakeRect(0, 0, size.width, size.height);
	
	if (size.width == 0 || size.height == 0)
		return nil;
	
	CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef bitmap = CGBitmapContextCreate(NULL, size.width, size.height, 8, 0, colorspace, kCGImageAlphaPremultipliedLast);
	NSGraphicsContext* context = [NSGraphicsContext graphicsContextWithCGContext:bitmap flipped:NO];
	
	[NSGraphicsContext setCurrentContext:context];
	[img drawInRect:rect];
	
	MTLPixelFormat format = (srgb ? MTLPixelFormatRGBA8Unorm_sRGB : MTLPixelFormatRGBA8Unorm);
	MTLTextureDescriptor* texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:size.width height:size.height mipmapped:YES];
	
	ret = [device newTextureWithDescriptor:texdesc];
	
	MTLRegion region;
	size_t bytesperrow = CGBitmapContextGetBytesPerRow(bitmap);
	
	region.origin.x = region.origin.y = region.origin.z = 0;
	
	region.size.width = size.width;
	region.size.height = size.height;
	region.size.depth = 1;
	
	[ret replaceRegion:region mipmapLevel:0 withBytes:CGBitmapContextGetData(bitmap) bytesPerRow:bytesperrow];

	if (ret.mipmapLevelCount > 1) {
		MetalTemporaryCommandBuffer(queue, true, [=](id<MTLCommandBuffer> transferdmc) -> bool {
			id<MTLBlitCommandEncoder> blitencoder = [transferdmc blitCommandEncoder];

			[blitencoder generateMipmapsForTexture:ret];
			[blitencoder endEncoding];
			
			return true;
		});
	}
	
	[NSGraphicsContext setCurrentContext:nil];

	CGContextRelease(bitmap);
	CGColorSpaceRelease(colorspace);
	
	printf("Loaded %s\n", name.c_str());
	MetalContentManager().RegisterTexture(file, ret);
	
	return ret;
}

id<MTLTexture> MetalCreateTextureFromDDS(id<MTLDevice> device, id<MTLCommandQueue> queue, const char* file, bool srgb)
{
	std::string name;
	Math::GetFile(name, file);
	
	id<MTLTexture> ret = MetalContentManager().PointerTexture(name);
	
	if (ret != nil) {
		printf("Pointer %s\n", file);
		return ret;
	}
	
	DDS_Image_Info info;
	MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
	
	if (!LoadFromDDS(file, &info)) {
		printf("Could not load %s\n", file);
		return nil;
	}
	
	if (info.Type == DDSImageType2D) {
		bool compressed = (info.Format == MTLPixelFormatBC1_RGBA || info.Format == MTLPixelFormatBC3_RGBA);
		uint32_t bytes = 4;
		uint32_t format = info.Format;

		if (info.Format == MTLPixelFormatRG32Float) {
			bytes = 8;
		} else if (info.Format == MTLPixelFormatRG16Float) {
			bytes = 4;
		} else if (info.Format == MTLPixelFormatRGBA8Snorm) {
			// this is a cheat for now...
			format = MTLPixelFormatBGRA8Unorm;
			bytes = 4;
		} else if (info.Format == MTLPixelFormatBGRA8Unorm || info.Format == MTLPixelFormatRGBA8Unorm) {
			if (srgb) {
				// see the enum
				format = info.Format + 1;
			}

			bytes = 3;
			
			// TODO: have to pad the data
		} else if (compressed && srgb) {
			format = info.Format + 1;
		}
		
		desc.textureType		= MTLTextureType2D;
		desc.pixelFormat		= (MTLPixelFormat)format;
		desc.width				= info.Width;
		desc.height				= info.Height;
		desc.depth				= 1;
		desc.mipmapLevelCount	= info.MipLevels;
		desc.arrayLength		= 1;
		desc.cpuCacheMode		= MTLCPUCacheModeWriteCombined;
		desc.storageMode		= MTLStorageModeManaged;
		desc.usage				= MTLTextureUsageShaderRead;
		desc.sampleCount		= 1;
		
		[desc setResourceOptions:(desc.cpuCacheMode << MTLResourceCPUCacheModeShift)|(desc.storageMode << MTLResourceStorageModeShift)];
		ret = [device newTextureWithDescriptor:desc];
		
		MTLRegion region;
		uint32_t pow2w = Math::NextPow2(info.Width);
		uint32_t pow2h = Math::NextPow2(info.Height);
		uint32_t width = info.Width;
		uint32_t height = info.Height;
		uint32_t offset = 0;
		uint32_t mipsize;
		
		region.origin = { 0, 0, 0 };
		region.size.depth = 1;
		
		for (int j = 0; j < info.MipLevels; ++j) {
			if (compressed)
				mipsize = GetCompressedLevelSize(info.Width, info.Height, j, info.Format);
			else
				mipsize = info.Width * info.Height * bytes;

			region.size.width = width;
			region.size.height = height;
			
			size_t pitch = width * bytes;
			size_t slicepitch = pitch * height;
			
			[ret replaceRegion:region mipmapLevel:j slice:0 withBytes:((uint8_t*)info.Data + offset) bytesPerRow:pitch bytesPerImage:slicepitch];
			
			offset += mipsize;
			
			width = (pow2w >> (j + 1));
			height = (pow2h >> (j + 1));
		}
	} else if (info.Type == DDSImageTypeCube) {
		if (info.Format != MTLPixelFormatRGBA16Float) {
			printf("MetalCreateTextureFromDDS(): Only RGBA16Float format is supported for cube maps\n");
			free(info.Data);

			return nil;
		}
		
		desc.textureType		= MTLTextureTypeCube;
		desc.pixelFormat		= (MTLPixelFormat)info.Format;
		desc.width				= info.Width;
		desc.height				= info.Height;
		desc.depth				= 1;
		desc.mipmapLevelCount	= info.MipLevels;
		desc.arrayLength		= 1;
		desc.cpuCacheMode		= MTLCPUCacheModeWriteCombined;
		desc.storageMode		= MTLStorageModeManaged;
		desc.usage				= MTLTextureUsageShaderRead;
		desc.sampleCount		= 1;
		
		[desc setResourceOptions:(desc.cpuCacheMode << MTLResourceCPUCacheModeShift)|(desc.storageMode << MTLResourceStorageModeShift)];
		ret = [device newTextureWithDescriptor:desc];
		
		MTLRegion region;
		size_t facesize;
		size_t offset = 0;

		region.origin = { 0, 0, 0 };
		region.size.depth = 1;
		
		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < info.MipLevels; ++j) {
				size_t size = Math::Max<size_t>(1, info.Width >> j);
				facesize = size * size * 8;

				region.size.width = region.size.height = size;
				
				size_t pitch = size * 8;
				size_t slicepitch = pitch * size;
				
				[ret replaceRegion:region mipmapLevel:j slice:i withBytes:((uint8_t*)info.Data + offset) bytesPerRow:pitch bytesPerImage:slicepitch];
				
				offset += facesize;
			}
		}
	} else {
		printf("MetalCreateTextureFromDDS(): This texture type is not supported for now\n");
		free(info.Data);

		return nil;
	}
	
	free(info.Data);
	
	printf("Loaded %s\n", name.c_str());
	MetalContentManager().RegisterTexture(file, ret);
	
	return ret;
}
