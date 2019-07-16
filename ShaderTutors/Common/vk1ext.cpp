
#include "vk1ext.h"
#include "3Dmath.h"
#include "geometryutils.h"
#include "dds.h"

#include <GlslangToSpv.h>

#ifdef _WIN32
#	include <Windows.h>
#	include <gdiplus.h>
#endif

#define DEBUG_SUBALLOCATOR

VulkanDriverInfo driverInfo;

// NOTE: needed for glslang; define here for easier modification
const TBuiltInResource SPIRVResources = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,
	/* .maxMeshOutputVerticesNV = */ 256,
	/* .maxMeshOutputPrimitivesNV = */ 512,
	/* .maxMeshWorkGroupSizeX_NV = */ 32,
	/* .maxMeshWorkGroupSizeY_NV = */ 1,
	/* .maxMeshWorkGroupSizeZ_NV = */ 1,
	/* .maxTaskWorkGroupSizeX_NV = */ 32,
	/* .maxTaskWorkGroupSizeY_NV = */ 1,
	/* .maxTaskWorkGroupSizeZ_NV = */ 1,
	/* .maxMeshViewCountNV = */ 4,

	/* .limits = */ {
		/* .nonInductiveForLoops = */ 1,
		/* .whileLoops = */ 1,
		/* .doWhileLoops = */ 1,
		/* .generalUniformIndexing = */ 1,
		/* .generalAttributeMatrixVectorIndexing = */ 1,
		/* .generalVaryingIndexing = */ 1,
		/* .generalSamplerIndexing = */ 1,
		/* .generalVariableIndexing = */ 1,
		/* .generalConstantMatrixVectorIndexing = */ 1,
	}
};

static EShLanguage FindLanguage(VkShaderStageFlagBits type)
{
	switch (type) {
	case VK_SHADER_STAGE_VERTEX_BIT:					return EShLangVertex;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:		return EShLangTessControl;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:	return EShLangTessEvaluation;
	case VK_SHADER_STAGE_GEOMETRY_BIT:					return EShLangGeometry;
	case VK_SHADER_STAGE_FRAGMENT_BIT:					return EShLangFragment;
	case VK_SHADER_STAGE_COMPUTE_BIT:					return EShLangCompute;
	default:											return EShLangVertex;
	}
}

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

#ifdef _WIN32
static Gdiplus::Bitmap* Win32LoadPicture(const std::string& file)
{
	std::wstring wstr;
	int size = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), 0, 0);

	wstr.resize(size);
	MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), &wstr[0], size);

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wstr.c_str(), FALSE);

	if (bitmap->GetLastStatus() != Gdiplus::Ok) {
		delete bitmap;
		bitmap = nullptr;
	}

	return bitmap;
}
#endif

// --- VulkanSubAllocation impl -----------------------------------------------

VulkanSubAllocation::VulkanSubAllocation()
{
	memory	= NULL;
	offset	= 0;
	heap	= VK_MAX_MEMORY_HEAPS;
}

VulkanMaterial::VulkanMaterial()
{
	texture = nullptr;
	normalMap = nullptr;
}

VulkanMaterial::~VulkanMaterial()
{
	VK_SAFE_RELEASE(texture);
	VK_SAFE_RELEASE(normalMap);
}

// --- VulkanMemorySubAllocator impl ------------------------------------------

VulkanMemorySubAllocator* VulkanMemorySubAllocator::_inst = nullptr;

const VulkanMemorySubAllocator::AllocationRecord& VulkanMemorySubAllocator::MemoryBatch::FindRecord(const VulkanSubAllocation& alloc) const
{
	AllocationSet::const_iterator it = allocations.find(alloc.offset);
	VK_ASSERT(it != allocations.end());

	return *it;
}

VulkanMemorySubAllocator& VulkanMemorySubAllocator::Instance()
{
	if (_inst == nullptr)
		_inst = new VulkanMemorySubAllocator();

	return *_inst;
}

void VulkanMemorySubAllocator::Release()
{
	if (_inst != nullptr)
		delete _inst;

	_inst = nullptr;
}

VulkanMemorySubAllocator::VulkanMemorySubAllocator()
{
}

VulkanMemorySubAllocator::~VulkanMemorySubAllocator()
{
}

VkDeviceSize VulkanMemorySubAllocator::AdjustBufferOffset(VkDeviceSize offset, VkDeviceSize alignment, VkBufferUsageFlags usageflags)
{
	VkDeviceSize minalignment = 4;

	if (usageflags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
		minalignment = driverInfo.deviceProps.limits.minTexelBufferOffsetAlignment;
	else if (usageflags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		minalignment = driverInfo.deviceProps.limits.minUniformBufferOffsetAlignment;
	else if (usageflags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		minalignment = driverInfo.deviceProps.limits.minStorageBufferOffsetAlignment;

	VK_ASSERT((alignment % minalignment) == 0);
	return ((offset % alignment) ? (offset + alignment - (offset % alignment)) : offset);
}

VkDeviceSize VulkanMemorySubAllocator::AdjustImageOffset(VkDeviceSize offset, VkDeviceSize alignment)
{
	return ((offset % alignment) ? (offset + alignment - (offset % alignment)) : offset);
}

const VulkanMemorySubAllocator::MemoryBatch& VulkanMemorySubAllocator::FindBatchForAlloc(const VulkanSubAllocation& alloc)
{
	MemoryBatch temp;
	size_t index;

	temp.memory = alloc.memory;
	index = batchesforheap[alloc.heap].Find(temp);

	VK_ASSERT(index != MemoryBatchArray::npos);
	return batchesforheap[alloc.heap][index];
}

const VulkanMemorySubAllocator::MemoryBatch& VulkanMemorySubAllocator::FindSuitableBatch(VkDeviceSize& outoffset, VkMemoryRequirements memreqs, VkFlags requirements, VkFlags usageflags, bool optimal)
{
	uint32_t index = GetMemoryTypeForFlags(memreqs.memoryTypeBits, requirements);
	VK_ASSERT(index < VK_MAX_MEMORY_HEAPS);

	MemoryBatchArray&	batches		= batchesforheap[index];
	VkDeviceSize		emptyspace	= 0;
	VkDeviceSize		offset		= 0;
	size_t				batchid		= SIZE_MAX;

	// find first hole which is good
	for (size_t i = 0; i < batches.Size(); ++i) {
		MemoryBatch& batch = (MemoryBatch&)batches[i];
		AllocationSet& allocs = batch.allocations;

		if (batch.isoptimal != optimal)
			continue;

		AllocationSet::iterator prev = allocs.begin();
		AllocationSet::iterator it = allocs.begin();

		if (prev == allocs.end()) {
			// batch is empty
			emptyspace = batches[i].totalsize;
			offset = 0;
			
			if (emptyspace >= memreqs.size) {
				batchid = i;
				break;
			}
		} else while (prev != allocs.end()) {
			if (prev == it) {
				// space before first alloc
				emptyspace = it->offset;
				offset = 0;

				++it;
			} else if (it == allocs.end()) {
				// space after last alloc
				if (optimal)
					offset = AdjustImageOffset(prev->offset + prev->size, memreqs.alignment);
				else
					offset = AdjustBufferOffset(prev->offset + prev->size, memreqs.alignment, usageflags);

				if (batches[i].totalsize > offset)
					emptyspace = batches[i].totalsize - offset;
				else
					emptyspace = 0;

				prev = it;
			} else {
				// space between allocs
				if (optimal)
					offset = AdjustImageOffset(prev->offset + prev->size, memreqs.alignment);
				else
					offset = AdjustBufferOffset(prev->offset + prev->size, memreqs.alignment, usageflags);

				if (it->offset > offset)
					emptyspace = it->offset - offset;
				else
					emptyspace = 0;

				prev = it;
				++it;
			}

			if (emptyspace >= memreqs.size) {
				// found a good spot
				batchid = i;
				break;
			}
		}

		if (batchid != SIZE_MAX)
			break;
	}

	if (batchid < batches.Size()) {
		// found suitable batch
		MemoryBatch& batch = (MemoryBatch&)batches[batchid];
		AllocationRecord record;

		VK_ASSERT(batch.isoptimal == optimal);

		record.offset	= offset;			// must be local (see 'emptyspace')
		record.size		= memreqs.size;

		outoffset = record.offset;
		batch.allocations.insert(record);

#ifdef DEBUG_SUBALLOCATOR
		if (optimal)
			printf("Optimal memory allocated in batch %llu (offset = %llu, size = %llu)\n", batchid, record.offset, record.size);
		else
			printf("Linear memory allocated in batch %llu (offset = %llu, size = %llu)\n", batchid, record.offset, record.size);
#endif

		return batch;
	}

	// create a new batch if possible
	VkDeviceSize maxsize = driverInfo.memoryProps.memoryHeaps[index].size;
	VkDeviceSize allocatedsize = 0;

	if (maxsize == 0)
		maxsize = SIZE_MAX;

	for (size_t i = 0; i < batches.Size(); ++i)
		allocatedsize += batches[i].totalsize;

	if (allocatedsize + memreqs.size > maxsize)
		throw std::bad_alloc(); /* "Memory heap is out of memory" */

	MemoryBatch				newbatch;
	AllocationRecord		record;
	VkMemoryAllocateInfo	allocinfo = {};
	VkResult				res;

	allocinfo.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocinfo.pNext				= NULL;
	allocinfo.memoryTypeIndex	= index;
	allocinfo.allocationSize	= Math::Max<VkDeviceSize>(memreqs.size, 64 * 1024 * 1024);

	// allocate in 64 MB chunks
	res = vkAllocateMemory(driverInfo.device, &allocinfo, NULL, &newbatch.memory);

	if (res != VK_SUCCESS)
		throw std::bad_alloc(); /* "Memory heap is out of memory" */

	newbatch.mappedcount	= 0;
	newbatch.totalsize		= allocinfo.allocationSize;
	newbatch.mappedrange	= nullptr;
	newbatch.isoptimal		= optimal;
	newbatch.heapindex		= index;

	record.offset			= 0;
	record.size				= memreqs.size;

	newbatch.allocations.insert(record);
	outoffset = record.offset;

	MemoryBatchArray::pairib result = batches.Insert(newbatch);
	VK_ASSERT(result.second);

#ifdef DEBUG_SUBALLOCATOR
	if (optimal)
		printf("Optimal memory allocated in batch %llu (offset = %llu, size = %llu)\n", result.first, record.offset, record.size);
	else
		printf("Linear memory allocated in batch %llu (offset = %llu, size = %llu)\n", result.first, record.offset, record.size);
#endif

	return (MemoryBatch&)batches[result.first];
}

VulkanSubAllocation VulkanMemorySubAllocator::AllocateForBuffer(VkMemoryRequirements memreqs, VkFlags requirements, VkBufferUsageFlags usageflags)
{
	VulkanSubAllocation result;
	const MemoryBatch& batch = FindSuitableBatch(result.offset, memreqs, requirements, usageflags, false);

	result.memory	= batch.memory;
	result.heap		= batch.heapindex;

	return result;
}

VulkanSubAllocation VulkanMemorySubAllocator::AllocateForImage(VkMemoryRequirements memreqs, VkFlags requirements, VkImageTiling tiling)
{
	VulkanSubAllocation result;
	const MemoryBatch& batch = FindSuitableBatch(result.offset, memreqs, requirements, 0, (tiling == VK_IMAGE_TILING_OPTIMAL));

	result.memory	= batch.memory;
	result.heap		= batch.heapindex;

	return result;
}

void VulkanMemorySubAllocator::Deallocate(VulkanSubAllocation& alloc)
{
	MemoryBatch& batch = (MemoryBatch&)FindBatchForAlloc(alloc);
	const AllocationRecord& record = batch.FindRecord(alloc);

	batch.allocations.erase(record);

	if (batch.allocations.empty()) {
		vkFreeMemory(driverInfo.device, batch.memory, NULL);
		batchesforheap[alloc.heap].Erase(batch);
	}

	alloc.memory = NULL;
	alloc.offset = 0;
}

uint32_t VulkanMemorySubAllocator::GetMemoryTypeForFlags(uint32_t memtype, VkFlags requirements)
{
	for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i) {
		if (memtype & 1) {
			if ((driverInfo.memoryProps.memoryTypes[i].propertyFlags & requirements) == requirements)
				return i;
		}

		memtype >>= 1;
	}

	return UINT32_MAX;
}

void* VulkanMemorySubAllocator::MapMemory(const VulkanSubAllocation& alloc, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
	const MemoryBatch& batch = FindBatchForAlloc(alloc);
	const AllocationRecord& record = batch.FindRecord(alloc);

	VK_ASSERT(!record.mapped);
	(void)size;

	// NOTE: vkMapMemory can't be nested
	uint8_t* ret = 0;

	if (batch.mappedcount == 0)
		vkMapMemory(driverInfo.device, alloc.memory, 0, VK_WHOLE_SIZE, flags, (void**)&batch.mappedrange);

	ret = batch.mappedrange + record.offset + offset;

	record.mapped = true;
	++batch.mappedcount;

	return ret;
}

void VulkanMemorySubAllocator::UnmapMemory(const VulkanSubAllocation& alloc)
{
	const MemoryBatch& batch = FindBatchForAlloc(alloc);
	const AllocationRecord& record = batch.FindRecord(alloc);

	VK_ASSERT(record.mapped);
	VK_ASSERT(batch.mappedcount > 0);

	record.mapped = false;
	--batch.mappedcount;

	if (batch.mappedcount == 0) {
		vkUnmapMemory(driverInfo.device, batch.memory);
		batch.mappedrange = 0;
	}
}

// --- VulkanBuffer impl ------------------------------------------------------

VulkanBuffer::VulkanBuffer()
{
	buffer			= NULL;
	stagingbuffer	= NULL;
	originalsize	= 0;
	exflags			= 0;
	contents		= nullptr;

	mappedrange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedrange.pNext = NULL;
}

VulkanBuffer::~VulkanBuffer()
{
	if (contents && stagingmemory)
		VulkanMemoryManager().UnmapMemory(stagingmemory);

	if (buffer)
		vkDestroyBuffer(driverInfo.device, buffer, 0);

	if (stagingbuffer)
		vkDestroyBuffer(driverInfo.device, stagingbuffer, 0);

	if (memory)
		VulkanMemoryManager().Deallocate(memory);

	if (stagingmemory)
		VulkanMemoryManager().Deallocate(stagingmemory);

	buffer		= NULL;
	contents	= nullptr;
}

VulkanBuffer* VulkanBuffer::Create(VkBufferUsageFlags usage, VkDeviceSize size, VkFlags flags)
{
	VkBufferCreateInfo	buffercreateinfo	= {};
	VulkanBuffer*		ret					= new VulkanBuffer();
	VkResult			res;
	bool				needsstaging		= ((flags & VK_MEMORY_PROPERTY_SHARED_BIT) == VK_MEMORY_PROPERTY_SHARED_BIT);

	ret->originalsize = size;
	ret->exflags = flags;

	// NOTE: memory must be aligned
	VkDeviceSize alignment = driverInfo.deviceProps.limits.nonCoherentAtomSize;
	size += (alignment - (size % alignment));

	// create device buffer first
	buffercreateinfo.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffercreateinfo.pNext					= NULL;
	buffercreateinfo.usage					= usage;
	buffercreateinfo.size					= size;
	buffercreateinfo.queueFamilyIndexCount	= 0;
	buffercreateinfo.pQueueFamilyIndices	= NULL;
	buffercreateinfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	buffercreateinfo.flags					= 0;

	if (needsstaging) {
		flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		buffercreateinfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	res = vkCreateBuffer(driverInfo.device, &buffercreateinfo, NULL, &ret->buffer);
	
	if (res != VK_SUCCESS) {
		delete ret;
		return nullptr;
	}

	vkGetBufferMemoryRequirements(driverInfo.device, ret->buffer, &ret->memreqs);

	if (needsstaging) {
		// turns out only here whether the memory really needs staging
		needsstaging = (UINT32_MAX == VulkanMemoryManager().GetMemoryTypeForFlags(ret->memreqs.memoryTypeBits, ret->exflags));

		if (!needsstaging)
			flags = ret->exflags;
	}

	ret->memory = VulkanMemoryManager().AllocateForBuffer(ret->memreqs, flags, buffercreateinfo.usage);

	if (!ret->memory) {
		delete ret;
		return nullptr;
	}

	res = vkBindBufferMemory(driverInfo.device, ret->buffer, ret->memory.memory, ret->memory.offset);

	if (res != VK_SUCCESS) {
		delete ret;
		return nullptr;
	}

	if (needsstaging) {
		VkMemoryRequirements stagingreqs;

		// create staging buffer
		flags = (ret->exflags & (~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		buffercreateinfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		res = vkCreateBuffer(driverInfo.device, &buffercreateinfo, NULL, &ret->stagingbuffer);
	
		if (res != VK_SUCCESS) {
			delete ret;
			return nullptr;
		}

		vkGetBufferMemoryRequirements(driverInfo.device, ret->stagingbuffer, &stagingreqs);
		ret->stagingmemory = VulkanMemoryManager().AllocateForBuffer(stagingreqs, flags, buffercreateinfo.usage);

		if (!ret->stagingmemory) {
			delete ret;
			return nullptr;
		}

		res = vkBindBufferMemory(driverInfo.device, ret->stagingbuffer, ret->stagingmemory.memory, ret->stagingmemory.offset);

		if (res != VK_SUCCESS) {
			delete ret;
			return nullptr;
		}
	}

	ret->bufferinfo.buffer	= ret->buffer;
	ret->bufferinfo.offset	= 0;
	ret->bufferinfo.range	= size;

	return ret;
}

void* VulkanBuffer::MapContents(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
	if (size == 0)
		size = memreqs.size;

	if (contents == nullptr) {
		if (stagingmemory) {
			contents = VulkanMemoryManager().MapMemory(stagingmemory, offset, size, flags);
			mappedrange.memory = stagingmemory.memory;
		} else {
			VK_ASSERT(exflags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			contents = VulkanMemoryManager().MapMemory(memory, offset, size, flags);
			mappedrange.memory = memory.memory;
		}

		mappedrange.offset = offset;
		mappedrange.size = size;
	}

	return contents;
}

void VulkanBuffer::UnmapContents()
{
	if (contents != nullptr) {
		if (!(exflags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			vkFlushMappedMemoryRanges(driverInfo.device, 1, &mappedrange);

		if (stagingmemory)
			VulkanMemoryManager().UnmapMemory(stagingmemory);
		else
			VulkanMemoryManager().UnmapMemory(memory);
	}

	contents = nullptr;
}

void VulkanBuffer::UploadToVRAM(VkCommandBuffer commandbuffer)
{
	if (stagingbuffer) {
		VkBufferCopy region;

		region.srcOffset	= 0;
		region.dstOffset	= 0;
		region.size			= originalsize;	// memreqs.size

		vkCmdCopyBuffer(commandbuffer, stagingbuffer, buffer, 1, &region);
	}
}

void VulkanBuffer::DeleteStagingBuffer()
{
	if (stagingbuffer)
		vkDestroyBuffer(driverInfo.device, stagingbuffer, 0);

	if (stagingmemory)
		VulkanMemoryManager().Deallocate(stagingmemory);

	stagingbuffer = NULL;
}

// --- VulkanRefCountable impl ------------------------------------------------

VulkanRefCountable::VulkanRefCountable()
{
	refcount = 1;
}

VulkanRefCountable::~VulkanRefCountable()
{
	assert(refcount == 0);
}

void VulkanRefCountable::AddRef()
{
	++refcount;
}

void VulkanRefCountable::Release()
{
	VK_ASSERT(refcount > 0);
	--refcount;

	if (refcount == 0)
		delete this;
}

// --- VulkanImage impl -------------------------------------------------------

VulkanImage::VulkanImage()
{
	static_assert(VK_IMAGE_LAYOUT_UNDEFINED == 0, "VK_IMAGE_LAYOUT_UNDEFINED must be 0");

	image					= NULL;
	imageview				= NULL;
	sampler					= NULL;
	format					= VK_FORMAT_UNDEFINED;
	accesses				= nullptr;
	layouts					= nullptr;
	stagingbuffer			= nullptr;
	mipmapcount				= 0;
	slicecount				= 1;

	imageinfo.imageLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	imageinfo.imageView		= NULL;
	imageinfo.sampler		= NULL;

	extents.width			= 1;
	extents.height			= 1;
	extents.depth			= 1;
}

VulkanImage::~VulkanImage()
{
	VulkanContentManager().UnregisterImage(this);

	delete[] accesses;
	delete[] layouts;

	if (stagingbuffer != nullptr)
		delete stagingbuffer;

	if (sampler)
		vkDestroySampler(driverInfo.device, sampler, 0);

	if (imageview)
		vkDestroyImageView(driverInfo.device, imageview, 0);

	if (image)
		vkDestroyImage(driverInfo.device, image, 0);

	if (memory)
		VulkanMemoryManager().Deallocate(memory);
}

VulkanImage* VulkanImage::Create2D(VkFormat format, uint32_t width, uint32_t height, uint32_t miplevels, VkImageUsageFlags usage)
{
	VkImageCreateInfo		imagecreateinfo		= {};
	VkImageViewCreateInfo	viewcreateinfo		= {};
	VkSamplerCreateInfo		samplercreateinfo	= {};
	VulkanImage*			ret					= new VulkanImage();
	VkFormatProperties		formatprops;
	VkFormatFeatureFlags	feature				= 0;
	VkImageAspectFlags		aspectmask			= 0;
	VkResult				res;

	ret->format = format;

	if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
		feature |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		aspectmask |= VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		feature |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		aspectmask |= VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		feature |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectmask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D32_SFLOAT_S8_UINT)
	{
		aspectmask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	vkGetPhysicalDeviceFormatProperties(driverInfo.selectedDevice, format, &formatprops);

	if (formatprops.optimalTilingFeatures & feature) {
		imagecreateinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	} else if (formatprops.linearTilingFeatures & feature) {
		imagecreateinfo.tiling = VK_IMAGE_TILING_LINEAR;
	} else {
		ret->Release();
		return NULL;
	}

	imagecreateinfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imagecreateinfo.pNext					= NULL;
	imagecreateinfo.arrayLayers				= 1;
	imagecreateinfo.extent.width			= width;
	imagecreateinfo.extent.height			= height;
	imagecreateinfo.extent.depth			= 1;
	imagecreateinfo.format					= format;
	imagecreateinfo.imageType				= VK_IMAGE_TYPE_2D;
	imagecreateinfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imagecreateinfo.mipLevels				= miplevels;
	imagecreateinfo.queueFamilyIndexCount	= 0;
	imagecreateinfo.pQueueFamilyIndices		= NULL;
	imagecreateinfo.samples					= VK_SAMPLE_COUNT_1_BIT;
	imagecreateinfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imagecreateinfo.usage					= usage;
	imagecreateinfo.flags					= 0;

	if (miplevels == 0) {
		VulkanImage::CalculateImageSizeAndMipmapCount(imagecreateinfo.mipLevels, imagecreateinfo.format, imagecreateinfo.extent.width, imagecreateinfo.extent.height);
		miplevels = imagecreateinfo.mipLevels;
	}

	ret->extents = imagecreateinfo.extent;
	ret->mipmapcount = miplevels;

	ret->accesses = new VkAccessFlags[ret->mipmapcount * imagecreateinfo.arrayLayers];
	ret->layouts = new VkImageLayout[ret->mipmapcount * imagecreateinfo.arrayLayers];

	memset(ret->accesses, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkAccessFlags));
	memset(ret->layouts, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkImageLayout));

	res = vkCreateImage(driverInfo.device, &imagecreateinfo, 0, &ret->image);

	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	vkGetImageMemoryRequirements(driverInfo.device, ret->image, &ret->memreqs);
	ret->memory = VulkanMemoryManager().AllocateForImage(ret->memreqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imagecreateinfo.tiling);
	
	if (!ret->memory) {
		delete ret;
		return NULL;
	}

	res = vkBindImageMemory(driverInfo.device, ret->image, ret->memory.memory, ret->memory.offset);
	
	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	viewcreateinfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewcreateinfo.pNext							= NULL;
	viewcreateinfo.components.r						= VK_COMPONENT_SWIZZLE_R;
	viewcreateinfo.components.g						= VK_COMPONENT_SWIZZLE_G;
	viewcreateinfo.components.b						= VK_COMPONENT_SWIZZLE_B;
	viewcreateinfo.components.a						= VK_COMPONENT_SWIZZLE_A;
	viewcreateinfo.format							= format;
	viewcreateinfo.image							= ret->image;
	viewcreateinfo.subresourceRange.aspectMask		= aspectmask;
	viewcreateinfo.subresourceRange.baseArrayLayer	= 0;
	viewcreateinfo.subresourceRange.baseMipLevel	= 0;
	viewcreateinfo.subresourceRange.layerCount		= 1;
	viewcreateinfo.subresourceRange.levelCount		= miplevels;
	viewcreateinfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
	viewcreateinfo.flags							= 0;

	res = vkCreateImageView(driverInfo.device, &viewcreateinfo, 0, &ret->imageview);

	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
		samplercreateinfo.sType				= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplercreateinfo.magFilter			= VK_FILTER_LINEAR;
		samplercreateinfo.minFilter			= VK_FILTER_LINEAR;
		samplercreateinfo.mipmapMode		= (miplevels == 1 ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR);
		samplercreateinfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplercreateinfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplercreateinfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplercreateinfo.mipLodBias		= 0.0;
		samplercreateinfo.anisotropyEnable	= VK_FALSE,
		samplercreateinfo.maxAnisotropy		= 0;
		samplercreateinfo.compareOp			= VK_COMPARE_OP_NEVER;
		samplercreateinfo.minLod			= 0.0f;
		samplercreateinfo.maxLod			= (float)imagecreateinfo.mipLevels;
		samplercreateinfo.compareEnable		= VK_FALSE;
		samplercreateinfo.borderColor		= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		res = vkCreateSampler(driverInfo.device, &samplercreateinfo, NULL, &ret->sampler);
	
		if (res != VK_SUCCESS) {
			delete ret;
			return NULL;
		}
	}

	ret->imageinfo.sampler		= ret->sampler;
	ret->imageinfo.imageView	= ret->imageview;
	ret->imageinfo.imageLayout	= imagecreateinfo.initialLayout;

	return ret;
}

VulkanImage* VulkanImage::CreateFromFile(const char* file, bool srgb)
{
	VulkanImage* ret = VulkanContentManager().PointerImage(file);
	uint8_t* imgdata = nullptr;

	if (ret != nullptr) {
		printf("Pointer %s\n", file);
		return ret;
	}

	VkImageCreateInfo		imagecreateinfo		= {};
	VkImageViewCreateInfo	viewcreateinfo		= {};
	VkSamplerCreateInfo		samplercreateinfo	= {};
	VkFormatProperties		formatprops;
	VkResult				res;

#ifdef _WIN32
	Gdiplus::Bitmap* bitmap = Win32LoadPicture(file);

	if (bitmap == nullptr)
		return nullptr;

	if (bitmap->GetLastStatus() != Gdiplus::Ok) {
		delete bitmap;
		return nullptr;
	}

	Gdiplus::BitmapData data;

	bitmap->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	{
		imgdata = new uint8_t[data.Width * data.Height * 4];
		memcpy(imgdata, data.Scan0, data.Width * data.Height * 4);
	}
	bitmap->UnlockBits(&data);

	imagecreateinfo.extent.width			= data.Width;
	imagecreateinfo.extent.height			= data.Height;

	delete bitmap;
#else
	// TODO:
#endif

	ret = new VulkanImage();

	vkGetPhysicalDeviceFormatProperties(driverInfo.selectedDevice, VK_FORMAT_B8G8R8A8_UNORM, &formatprops);

	imagecreateinfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imagecreateinfo.pNext					= NULL;
	imagecreateinfo.arrayLayers				= 1;
	imagecreateinfo.extent.depth			= 1;
	imagecreateinfo.format					= (srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM);
	imagecreateinfo.imageType				= VK_IMAGE_TYPE_2D;
	imagecreateinfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imagecreateinfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imagecreateinfo.queueFamilyIndexCount	= 0;
	imagecreateinfo.pQueueFamilyIndices		= NULL;
	imagecreateinfo.samples					= VK_SAMPLE_COUNT_1_BIT;
	imagecreateinfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imagecreateinfo.usage					= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	imagecreateinfo.flags					= 0;

	VulkanImage::CalculateImageSizeAndMipmapCount(imagecreateinfo.mipLevels, imagecreateinfo.format, imagecreateinfo.extent.width, imagecreateinfo.extent.height);

	ret->extents = imagecreateinfo.extent;
	ret->mipmapcount = imagecreateinfo.mipLevels;

	ret->accesses = new VkAccessFlags[ret->mipmapcount * imagecreateinfo.arrayLayers];
	ret->layouts = new VkImageLayout[ret->mipmapcount * imagecreateinfo.arrayLayers];

	memset(ret->accesses, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkAccessFlags));
	memset(ret->layouts, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkImageLayout));

	res = vkCreateImage(driverInfo.device, &imagecreateinfo, 0, &ret->image);

	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	vkGetImageMemoryRequirements(driverInfo.device, ret->image, &ret->memreqs);
	ret->memory = VulkanMemoryManager().AllocateForImage(ret->memreqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imagecreateinfo.tiling);

	if (!ret->memory) {
		delete ret;
		return NULL;
	}

	res = vkBindImageMemory(driverInfo.device, ret->image, ret->memory.memory, ret->memory.offset);
	
	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	if (imgdata != nullptr) {
		size_t datasize = imagecreateinfo.extent.width * imagecreateinfo.extent.height * 4;

		ret->stagingbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, datasize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_ASSERT(ret->stagingbuffer != nullptr);

		void* memdata = ret->stagingbuffer->MapContents(0, 0);
		{
			memcpy(memdata, imgdata, datasize);
		}
		ret->stagingbuffer->UnmapContents();

		delete[] imgdata;
	}

	viewcreateinfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewcreateinfo.pNext							= NULL;
	viewcreateinfo.components.r						= VK_COMPONENT_SWIZZLE_R;
	viewcreateinfo.components.g						= VK_COMPONENT_SWIZZLE_G;
	viewcreateinfo.components.b						= VK_COMPONENT_SWIZZLE_B;
	viewcreateinfo.components.a						= VK_COMPONENT_SWIZZLE_A;
	viewcreateinfo.format							= imagecreateinfo.format;
	viewcreateinfo.image							= ret->image;
	viewcreateinfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	viewcreateinfo.subresourceRange.baseArrayLayer	= 0;
	viewcreateinfo.subresourceRange.baseMipLevel	= 0;
	viewcreateinfo.subresourceRange.layerCount		= 1;
	viewcreateinfo.subresourceRange.levelCount		= imagecreateinfo.mipLevels;
	viewcreateinfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
	viewcreateinfo.flags							= 0;

	res = vkCreateImageView(driverInfo.device, &viewcreateinfo, 0, &ret->imageview);

	if (res != VK_SUCCESS) {
		delete ret;
		return nullptr;
	}

	samplercreateinfo.sType				= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplercreateinfo.magFilter			= VK_FILTER_LINEAR;
	samplercreateinfo.minFilter			= VK_FILTER_LINEAR;
	samplercreateinfo.mipmapMode		= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplercreateinfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplercreateinfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplercreateinfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplercreateinfo.mipLodBias		= 0.0;
	samplercreateinfo.anisotropyEnable	= VK_FALSE,
	samplercreateinfo.maxAnisotropy		= 0;
	samplercreateinfo.compareOp			= VK_COMPARE_OP_NEVER;
	samplercreateinfo.minLod			= 0.0f;
	samplercreateinfo.maxLod			= (float)imagecreateinfo.mipLevels;
	samplercreateinfo.compareEnable		= VK_FALSE;
	samplercreateinfo.borderColor		= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	res = vkCreateSampler(driverInfo.device, &samplercreateinfo, NULL, &ret->sampler);
	
	if (res != VK_SUCCESS) {
		delete ret;
		return nullptr;
	}

	ret->imageinfo.sampler		= ret->sampler;
	ret->imageinfo.imageView	= ret->imageview;
	ret->imageinfo.imageLayout	= imagecreateinfo.initialLayout;

	printf("Loaded %s\n", file);
	VulkanContentManager().RegisterImage(file, ret);

	return ret;
}

VulkanImage* VulkanImage::CreateFromDDS(const char* file, bool srgb)
{
	VulkanImage* ret = VulkanContentManager().PointerImage(file);

	if (ret != nullptr) {
		printf("Pointer %s\n", file);
		return ret;
	}

	DDS_Image_Info			info;
	VkImageCreateInfo		imagecreateinfo		= {};
	VkImageViewCreateInfo	viewcreateinfo		= {};
	VkSamplerCreateInfo		samplercreateinfo	= {};
	VkFormatProperties		formatprops;
	VkResult				res;

	if (!LoadFromDDS(file, &info)) {
		printf("Could not load %s\n", file);
		return nullptr;
	}

	if (info.Type != DDSImageTypeCube) {
		printf("VulkanImage::CreateFromDDS(): Only cubemaps are supported for now\n");
		return nullptr;
	}

	ret = new VulkanImage();
	vkGetPhysicalDeviceFormatProperties(driverInfo.selectedDevice, VK_FORMAT_B8G8R8A8_UNORM, &formatprops);

	imagecreateinfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imagecreateinfo.pNext					= NULL;
	imagecreateinfo.arrayLayers				= 6;
	imagecreateinfo.extent.width			= info.Width;
	imagecreateinfo.extent.height			= info.Height;
	imagecreateinfo.extent.depth			= 1;
	imagecreateinfo.format					= (srgb ? VK_FORMAT_B8G8R8A8_SRGB : (VkFormat)info.Format);
	imagecreateinfo.imageType				= VK_IMAGE_TYPE_2D;
	imagecreateinfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imagecreateinfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imagecreateinfo.queueFamilyIndexCount	= 0;
	imagecreateinfo.pQueueFamilyIndices		= NULL;
	imagecreateinfo.samples					= VK_SAMPLE_COUNT_1_BIT;
	imagecreateinfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imagecreateinfo.usage					= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	imagecreateinfo.flags					= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VulkanImage::CalculateImageSizeAndMipmapCount(imagecreateinfo.mipLevels, imagecreateinfo.format, imagecreateinfo.extent.width, imagecreateinfo.extent.height);

	ret->extents = imagecreateinfo.extent;
	ret->mipmapcount = imagecreateinfo.mipLevels;
	ret->slicecount = imagecreateinfo.arrayLayers;

	ret->accesses = new VkAccessFlags[ret->mipmapcount * imagecreateinfo.arrayLayers];
	ret->layouts = new VkImageLayout[ret->mipmapcount * imagecreateinfo.arrayLayers];

	memset(ret->accesses, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkAccessFlags));
	memset(ret->layouts, 0, ret->mipmapcount * imagecreateinfo.arrayLayers * sizeof(VkImageLayout));

	res = vkCreateImage(driverInfo.device, &imagecreateinfo, 0, &ret->image);

	if (res != VK_SUCCESS) {
		delete ret;
		free(info.Data);

		return NULL;
	}

	vkGetImageMemoryRequirements(driverInfo.device, ret->image, &ret->memreqs);
	ret->memory = VulkanMemoryManager().AllocateForImage(ret->memreqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imagecreateinfo.tiling);

	if (!ret->memory) {
		delete ret;
		free(info.Data);

		return NULL;
	}

	res = vkBindImageMemory(driverInfo.device, ret->image, ret->memory.memory, ret->memory.offset);
	
	if (res != VK_SUCCESS) {
		delete ret;
		free(info.Data);

		return NULL;
	}

	size_t slicesize = CalculateSliceSize((VkFormat)info.Format, info.Width, info.Height);

	ret->stagingbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 6 * slicesize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_ASSERT(ret->stagingbuffer);

	void* memdata = ret->stagingbuffer->MapContents(0, 0);
	memcpy(memdata, info.Data, 6 * slicesize);

	ret->stagingbuffer->UnmapContents();
	free(info.Data);

	viewcreateinfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewcreateinfo.pNext							= NULL;
	viewcreateinfo.components.r						= VK_COMPONENT_SWIZZLE_R;
	viewcreateinfo.components.g						= VK_COMPONENT_SWIZZLE_G;
	viewcreateinfo.components.b						= VK_COMPONENT_SWIZZLE_B;
	viewcreateinfo.components.a						= VK_COMPONENT_SWIZZLE_A;
	viewcreateinfo.format							= imagecreateinfo.format;
	viewcreateinfo.image							= ret->image;
	viewcreateinfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	viewcreateinfo.subresourceRange.baseArrayLayer	= 0;
	viewcreateinfo.subresourceRange.baseMipLevel	= 0;
	viewcreateinfo.subresourceRange.layerCount		= 6;
	viewcreateinfo.subresourceRange.levelCount		= imagecreateinfo.mipLevels;
	viewcreateinfo.viewType							= VK_IMAGE_VIEW_TYPE_CUBE;
	viewcreateinfo.flags							= 0;

	res = vkCreateImageView(driverInfo.device, &viewcreateinfo, 0, &ret->imageview);

	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	samplercreateinfo.sType				= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplercreateinfo.magFilter			= VK_FILTER_LINEAR;
	samplercreateinfo.minFilter			= VK_FILTER_LINEAR;
	samplercreateinfo.mipmapMode		= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplercreateinfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplercreateinfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplercreateinfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplercreateinfo.mipLodBias		= 0.0;
	samplercreateinfo.anisotropyEnable	= VK_FALSE,
	samplercreateinfo.maxAnisotropy		= 0;
	samplercreateinfo.compareOp			= VK_COMPARE_OP_NEVER;
	samplercreateinfo.minLod			= 0.0f;
	samplercreateinfo.maxLod			= (float)imagecreateinfo.mipLevels;
	samplercreateinfo.compareEnable		= VK_FALSE;
	samplercreateinfo.borderColor		= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	res = vkCreateSampler(driverInfo.device, &samplercreateinfo, NULL, &ret->sampler);
	
	if (res != VK_SUCCESS) {
		delete ret;
		return NULL;
	}

	ret->imageinfo.sampler		= ret->sampler;
	ret->imageinfo.imageView	= ret->imageview;
	ret->imageinfo.imageLayout	= imagecreateinfo.initialLayout;

	printf("Loaded %s\n", file);
	VulkanContentManager().RegisterImage(file, ret);

	return ret;
}

size_t VulkanImage::CalculateImageSizeAndMipmapCount(uint32_t& nummipsout, VkFormat format, uint32_t width, uint32_t height)
{
	nummipsout = Math::Max<uint32_t>(1, (uint32_t)floor(log(Math::Max<double>(width, height)) / 0.69314718055994530941723212));

	size_t w		= width;
	size_t h		= height;
	size_t bytesize	= 0;
	size_t bytes	= CalculateByteSize(format);

	for (uint32_t i = 0; i < nummipsout; ++i) {
		bytesize += Math::Max<size_t>(1, w) * Math::Max<size_t>(1, h) * bytes;

		w = Math::Max<size_t>(w / 2, 1);
		h = Math::Max<size_t>(h / 2, 1);
	}

	return bytesize;
}

size_t VulkanImage::CalculateSliceSize(VkFormat format, uint32_t width, uint32_t height)
{
	return width * height * CalculateByteSize(format);
}

size_t VulkanImage::CalculateByteSize(VkFormat format)
{
	size_t bytes = 0;

	// TODO: all formats
	switch (format) {
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SRGB:
		bytes = 4;
		break;

	case VK_FORMAT_R16G16B16A16_SFLOAT:
		bytes = 8;
		break;

	default:
		VK_ASSERT(false);
		break;
	}

	return bytes;
}

void* VulkanImage::MapContents(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
	if (stagingbuffer == nullptr) {
		stagingbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memreqs.size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_ASSERT(stagingbuffer);
	}

	return stagingbuffer->MapContents(offset, size, flags);
}

void VulkanImage::UnmapContents()
{
	VK_ASSERT(stagingbuffer);
	stagingbuffer->UnmapContents();
}

void VulkanImage::UploadToVRAM(VulkanPipelineBarrierBatch& barrier, VkCommandBuffer commandbuffer, bool generatemips)
{
	VK_ASSERT(stagingbuffer != nullptr);

	VkBufferImageCopy region;

	region.bufferOffset			= 0;
	region.bufferRowLength		= 0;
	region.bufferImageHeight	= 0;

	region.imageOffset.x		= 0;
	region.imageOffset.y		= 0;
	region.imageOffset.z		= 0;
	region.imageExtent			= extents;

	region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer	= 0;
	region.imageSubresource.layerCount		= slicecount;
	region.imageSubresource.mipLevel		= 0;

	// transition entire image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or we can't generate mipmaps
	barrier.ImageLayoutTransition(this, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0, 0);
	barrier.Enlist(commandbuffer);

	vkCmdCopyBufferToImage(commandbuffer, stagingbuffer->GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (generatemips) {
		VkImageBlit blit;
		uint32_t width, height;

		blit.srcOffsets[0].x = 0;
		blit.srcOffsets[0].y = 0;
		blit.srcOffsets[0].z = 0;

		blit.srcOffsets[1].x = extents.width;
		blit.srcOffsets[1].y = extents.height;
		blit.srcOffsets[1].z = extents.depth;

		blit.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer	= 0;
		blit.srcSubresource.layerCount		= slicecount;
		blit.srcSubresource.mipLevel		= 0;

		blit.dstOffsets[0].x = 0;
		blit.dstOffsets[0].y = 0;
		blit.dstOffsets[0].z = 0;

		blit.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer	= 0;
		blit.dstSubresource.layerCount		= slicecount;

		// NOTE: all levels must be in the same layout
		for (uint32_t i = 1; i <= mipmapcount; ++i) {
			// must transition previous level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL before blitting
			barrier.ImageLayoutTransition(this, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i - 1, 1, 0, 0);
			barrier.Enlist(commandbuffer);

			if (i < mipmapcount) {
				// now blit
				width = Math::Max<uint32_t>(1, extents.width >> i);
				height = Math::Max<uint32_t>(1, extents.height >> i);

				blit.dstOffsets[1].x = width;
				blit.dstOffsets[1].y = height;
				blit.dstOffsets[1].z = 1;
				blit.dstSubresource.mipLevel = i;

				vkCmdBlitImage(commandbuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
			}
		}
	}
}

void VulkanImage::DeleteStagingBuffer()
{
	if (stagingbuffer)
		delete stagingbuffer;

	stagingbuffer = nullptr;
}

VkAccessFlags VulkanImage::GetAccess(uint32_t level, uint32_t slice) const
{
	return accesses[level * slicecount + slice];
}

VkImageLayout VulkanImage::GetLayout(uint32_t level, uint32_t slice) const
{
	return layouts[level * slicecount + slice];
}

// --- VulkanMesh impl --------------------------------------------------------

VulkanMesh::VulkanMesh(uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, VulkanBuffer* buffer, VkDeviceSize offset, uint32_t flags)
{
	vertexbuffer	= nullptr;
	indexbuffer		= nullptr;
	uniformbuffer	= nullptr;
	subsettable		= nullptr;
	materials		= nullptr;

	totalsize		= numvertices * vertexstride;
	vertexcount		= numvertices;
	indexcount		= numindices;
	vstride			= vertexstride;
	baseoffset		= (buffer ? offset : 0);
	inherited		= (buffer != nullptr);
	numsubsets		= 1;

	// TODO: when primitive reset is disabled
	if (numvertices > 0xffff || (flags & VK_MESH_32BIT))
		indexformat = VK_INDEX_TYPE_UINT32;
	else
		indexformat = VK_INDEX_TYPE_UINT16;

	if (buffer != nullptr) {
		if (totalsize % 4 > 0)
			totalsize += (4 - (totalsize % 4));

		indexoffset = baseoffset + totalsize;
		totalsize += numindices * (indexformat == VK_INDEX_TYPE_UINT16 ? 2 : 4);

		if (totalsize % 256 > 0)
			totalsize += (256 - (totalsize % 256));

		uniformoffset = baseoffset + totalsize;
		totalsize += 512;	// 32 registers

		vertexbuffer = indexbuffer = uniformbuffer = buffer;
		VK_ASSERT(baseoffset + totalsize <= buffer->GetSize());

		mappedvdata = mappedidata = mappedudata = (uint8_t*)buffer->MapContents(0, 0);
	} else {
		totalsize += numindices * (indexformat == VK_INDEX_TYPE_UINT16 ? 2 : 4);

		vertexbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, numvertices * vertexstride, VK_MEMORY_PROPERTY_SHARED_BIT);
		indexbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, numindices * (indexformat == VK_INDEX_TYPE_UINT16 ? 2 : 4), VK_MEMORY_PROPERTY_SHARED_BIT);
		uniformbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 512, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		indexoffset = 0;
		uniformoffset = 0;

		mappedvdata = (uint8_t*)vertexbuffer->MapContents(0, 0);
		mappedidata = (uint8_t*)indexbuffer->MapContents(0, 0);
		mappedudata = (uint8_t*)uniformbuffer->MapContents(0, 0);
	}
	
	VK_ASSERT(vertexbuffer != nullptr);
	VK_ASSERT(indexbuffer != nullptr);
	VK_ASSERT(uniformbuffer != nullptr);

	unibufferinfo.buffer	= uniformbuffer->GetBuffer();
	unibufferinfo.offset	= uniformoffset;
	unibufferinfo.range		= 512;

	subsettable = new VulkanAttributeRange[1];
	materials = new VulkanMaterial[1];

	subsettable[0].attribId			= 0;
	subsettable[0].indexCount		= numindices;
	subsettable[0].indexStart		= 0;
	subsettable[0].primitiveType	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	subsettable[0].vertexCount		= numvertices;
	subsettable[0].vertexStart		= 0;
	subsettable[0].enabled			= true;

	materials[0].ambient			= Math::Color(0, 0, 0, 1);
	materials[0].diffuse			= Math::Color(1, 1, 1, 1);
	materials[0].specular			= Math::Color(1, 1, 1, 1);
	materials[0].emissive			= Math::Color(0, 0, 0, 1);
	materials[0].power				= 80;
	materials[0].texture			= nullptr;
	materials[0].normalMap			= nullptr;
}

VulkanMesh::~VulkanMesh()
{
	if (subsettable != nullptr)
		delete[] subsettable;

	if (materials != nullptr)
		delete[] materials;

	if (!inherited) {
		if (vertexbuffer != nullptr)
			delete vertexbuffer;

		if (indexbuffer != nullptr)
			delete indexbuffer;

		if (uniformbuffer != nullptr)
			delete uniformbuffer;
	}
}

void VulkanMesh::Draw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline, bool rebind)
{
	if (rebind) {
		VkBuffer vbuff = vertexbuffer->GetBuffer();

		vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vbuff, &baseoffset);
		vkCmdBindIndexBuffer(commandbuffer, indexbuffer->GetBuffer(), indexoffset, indexformat);
	}

	for (uint32_t i = 0; i < numsubsets; ++i)
		DrawSubset(commandbuffer, i, pipeline, false);
}

void VulkanMesh::DrawSubset(VkCommandBuffer commandbuffer, uint32_t index, VulkanGraphicsPipeline* pipeline, bool rebind)
{
	VK_ASSERT(index < numsubsets);

	const VulkanAttributeRange& subset = subsettable[index];

	if (subset.enabled) {
		if (rebind) {
			VkBuffer vbuff = vertexbuffer->GetBuffer();

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vbuff, &baseoffset);
			vkCmdBindIndexBuffer(commandbuffer, indexbuffer->GetBuffer(), indexoffset, indexformat);
		}

		if (pipeline != nullptr)
			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, pipeline->GetDescriptorSets(0) + index, 0, NULL);

		vkCmdDrawIndexed(commandbuffer, subset.indexCount, 1, subset.indexStart, 0, 0);
	}
}

void VulkanMesh::DrawSubsetInstanced(VkCommandBuffer commandbuffer, uint32_t index, VulkanGraphicsPipeline* pipeline, uint32_t numinstances, bool rebind)
{
	VK_ASSERT(index < numsubsets);

	const VulkanAttributeRange& subset = subsettable[index];

	if (subset.enabled) {
		if (rebind) {
			VkBuffer vbuff = vertexbuffer->GetBuffer();

			vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vbuff, &baseoffset);
			vkCmdBindIndexBuffer(commandbuffer, indexbuffer->GetBuffer(), indexoffset, indexformat);
		}

		if (pipeline != nullptr)
			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, pipeline->GetDescriptorSets(0) + index, 0, NULL);

		vkCmdDrawIndexed(commandbuffer, subset.indexCount, numinstances, subset.indexStart, 0, 0);
	}
}

void VulkanMesh::EnableSubset(uint32_t index, bool enable)
{
	VK_ASSERT(index < numsubsets);
	subsettable[index].enabled = enable;
}

void VulkanMesh::UploadToVRAM(VulkanPipelineBarrierBatch& barrier, VkCommandBuffer commandbuffer)
{
	if (!inherited && (mappedvdata != nullptr || mappedidata != nullptr)) {
		// unmap before use
		vertexbuffer->UnmapContents();

		if (indexbuffer != vertexbuffer)
			indexbuffer->UnmapContents();

		mappedvdata = mappedidata = nullptr;
	}

	if (inherited) {
		vertexbuffer->UploadToVRAM(commandbuffer);
	} else {
		vertexbuffer->UploadToVRAM(commandbuffer);
		indexbuffer->UploadToVRAM(commandbuffer);

		// NOTE: will access data from host memory which is slow
		//uniformbuffer->UploadToVRAM(commandbuffer);
	}

	for (uint32_t i = 0; i < numsubsets; ++i) {
		if (materials[i].texture != nullptr)
			materials[i].texture->UploadToVRAM(barrier, commandbuffer);

		if (materials[i].normalMap != nullptr)
			materials[i].normalMap->UploadToVRAM(barrier, commandbuffer);
	}

	barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	// NOTE: be careful not to transition twice
	for (uint32_t i = 0; i < numsubsets; ++i) {
		if (materials[i].texture != nullptr) {
			if (materials[i].texture->GetLayout(0, 0) != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				barrier.ImageLayoutTransition(materials[i].texture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, materials[i].texture->GetMipMapCount(), 0, materials[i].texture->GetArraySize());
		}

		if (materials[i].normalMap != nullptr) {
			if (materials[i].normalMap->GetLayout(0, 0) != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				barrier.ImageLayoutTransition(materials[i].normalMap, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, materials[i].normalMap->GetMipMapCount(), 0, materials[i].normalMap->GetArraySize());
		}
	}

	barrier.Enlist(commandbuffer);
}

void VulkanMesh::DeleteStagingBuffers()
{
	if (inherited) {
		vertexbuffer->DeleteStagingBuffer();
	} else {
		vertexbuffer->DeleteStagingBuffer();
		indexbuffer->DeleteStagingBuffer();

		// NOTE: don't delete
		//uniformbuffer->DeleteStagingBuffer();
	}

	for (uint32_t i = 0; i < numsubsets; ++i) {
		if (materials[i].texture != nullptr)
			materials[i].texture->DeleteStagingBuffer();

		if (materials[i].normalMap != nullptr)
			materials[i].normalMap->DeleteStagingBuffer();
	}
}

void VulkanMesh::GenerateTangentFrame()
{
	VK_ASSERT(vstride == sizeof(GeometryUtils::CommonVertex));
	VK_ASSERT(vertexbuffer != nullptr);
	VK_ASSERT(!inherited);

	VulkanBuffer*					newbuffer	= VulkanBuffer::Create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexcount * sizeof(GeometryUtils::TBNVertex), VK_MEMORY_PROPERTY_SHARED_BIT);
	GeometryUtils::CommonVertex*	oldvdata	= (GeometryUtils::CommonVertex*)vertexbuffer->MapContents(0, 0);
	GeometryUtils::TBNVertex*		newvdata	= (GeometryUtils::TBNVertex*)newbuffer->MapContents(0, 0);
	void*							idata		= indexbuffer->MapContents(0, 0);
	uint32_t						i1, i2, i3;
	bool							is32bit		= (indexformat == VK_INDEX_TYPE_UINT32);

	for (uint32_t i = 0; i < numsubsets; ++i) {
		const VulkanAttributeRange& subset = subsettable[i];
		VK_ASSERT(subset.indexCount > 0);

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

	indexbuffer->UnmapContents();
	newbuffer->UnmapContents();
	vertexbuffer->UnmapContents();

	delete vertexbuffer;

	vertexbuffer = newbuffer;
	vstride = sizeof(GeometryUtils::TBNVertex);
}

void* VulkanMesh::GetVertexBufferPointer()
{
	return (mappedvdata + baseoffset);
}

void* VulkanMesh::GetIndexBufferPointer()
{
	return (mappedidata + indexoffset);
}

void* VulkanMesh::GetUniformBufferPointer()
{
	return (mappedudata + uniformoffset);
}

VulkanMesh* VulkanMesh::LoadFromQM(const char* file, VulkanBuffer* buffer, VkDeviceSize offset)
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

	static VkFormat elemformats[] = {
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UINT
	};

	VkVertexInputAttributeDescription*	vertexlayout	= nullptr;	// TODO: make this a member
	VulkanMesh*							mesh			= nullptr;
	FILE*								infile			= nullptr;
	void*								data			= nullptr;
	
	std::string							basedir(file), str;
	Math::Vector3						bbmin, bbmax;
	char								buff[256];

	uint32_t							unused;
	uint32_t							version;
	uint32_t							numindices;
	uint32_t							numvertices;
	uint32_t							vstride;
	uint32_t							istride;
	uint32_t							numsubsets;
	uint32_t							numelems;
	uint16_t							tmp16;
	uint8_t								tmp8;
	uint8_t								elemtype;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb");
#endif

	if (!infile)
		return false;

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

	// vertex declaration
	fread(&numelems, 4, 1, infile);

	vstride = 0;
	vertexlayout = new VkVertexInputAttributeDescription[numelems];

	for (uint32_t i = 0; i < numelems; ++i) {
		vertexlayout[i].location = i;

		fread(&tmp16, 2, 1, infile);	// binding
		vertexlayout[i].binding = tmp16;

		fread(&tmp8, 1, 1, infile);		// usage
		fread(&elemtype, 1, 1, infile);	// type
		fread(&tmp8, 1, 1, infile);		// usageindex

		vertexlayout[i].offset = vstride;
		vertexlayout[i].format = elemformats[elemtype];

		vstride += elemsizes[elemtype] * elemstrides[elemtype];
	}

	mesh = new VulkanMesh(numvertices, numindices, vstride, buffer, offset);
	
	delete[] mesh->materials;
	delete[] mesh->subsettable;

	delete vertexlayout; //

	mesh->numsubsets = numsubsets;
	mesh->subsettable = new VulkanAttributeRange[numsubsets];
	mesh->materials = new VulkanMaterial[numsubsets];

	// data
	data = mesh->GetVertexBufferPointer();
	fread(data, vstride, numvertices, infile);

	data = mesh->GetIndexBufferPointer();
	fread(data, istride, numindices, infile);

	if (version >= 1) {
		fread(&unused, 4, 1, infile);

		if (unused > 0)
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	for (uint32_t i = 0; i < numsubsets; ++i) {
		VulkanAttributeRange& subset = mesh->subsettable[i];
		VulkanMaterial& material = mesh->materials[i];

		subset.attribId = i;
		subset.primitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
			fread(&unused, 4, 1, infile);		// blendmode

			ReadString(infile, buff);

			if (buff[1] != ',') {
				str = basedir + buff;
				material.texture = VulkanImage::CreateFromFile(str.c_str(), true);
			}

			ReadString(infile, buff);

			if (buff[1] != ',') {
				str = basedir + buff;
				material.normalMap = VulkanImage::CreateFromFile(str.c_str(), false);
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
			material.texture = VulkanImage::CreateFromFile(str.c_str(), true);
		}

		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
	}

	// printf some info
	Math::GetFile(str, file);
	mesh->boundingbox.GetSize(bbmin);

	printf("Loaded mesh '%s': size = (%.3f, %.3f, %.3f) [%u verts, %u tris]\n", str.c_str(), bbmin[0], bbmin[1], bbmin[2], numvertices, numindices / 3);

	fclose(infile);
	return mesh;
}

// --- VulkanContentRegistry impl ---------------------------------------------

VulkanContentRegistry* VulkanContentRegistry::_inst = nullptr;

VulkanContentRegistry& VulkanContentRegistry::Instance()
{
	if (_inst == nullptr)
		_inst = new VulkanContentRegistry();

	return *_inst;
}

void VulkanContentRegistry::Release()
{
	if (_inst != nullptr)
		delete _inst;

	_inst = nullptr;
}

VulkanContentRegistry::VulkanContentRegistry()
{
}

VulkanContentRegistry::~VulkanContentRegistry()
{
}

void VulkanContentRegistry::RegisterImage(const std::string& file, VulkanImage* image)
{
	std::string name;
	Math::GetFile(name, file);

	VK_ASSERT(images.count(name) == 0);
	images.insert(ImageMap::value_type(name, image));
}

void VulkanContentRegistry::UnregisterImage(VulkanImage* image)
{
	for (ImageMap::iterator it = images.begin(); it != images.end(); ++it) {
		if (it->second == image) {
			images.erase(it);
			break;
		}
	}
}

VulkanImage* VulkanContentRegistry::PointerImage(const std::string& file)
{
	std::string name;
	Math::GetFile(name, file);

	ImageMap::iterator it = images.find(name);

	if (it == images.end())
		return NULL;

	it->second->AddRef();
	return it->second;
}

// --- VulkanRenderPass impl --------------------------------------------------

VulkanRenderPass::VulkanRenderPass(uint32_t width, uint32_t height, uint32_t subpasses)
{
	renderpass			= NULL;
	clearcolors			= nullptr;
	framewidth			= width;
	frameheight			= height;
	numsubpasses		= subpasses;

	inputreferences		= new ReferenceArray[numsubpasses];
	colorreferences		= new ReferenceArray[numsubpasses];
	preservereferences	= new PreserveArray[numsubpasses];
	depthreferences		= new VkAttachmentReference[numsubpasses];

	memset(depthreferences, 0, numsubpasses * sizeof(VkAttachmentReference));
	attachmentdescs.reserve(5);
}

VulkanRenderPass::~VulkanRenderPass()
{
	delete[] clearcolors;
	delete[] inputreferences;
	delete[] colorreferences;
	delete[] preservereferences;
	delete[] depthreferences;

	if (renderpass)
		vkDestroyRenderPass(driverInfo.device, renderpass, NULL);
}

void VulkanRenderPass::AddAttachment(VkFormat format, VkAttachmentLoadOp loadop, VkAttachmentStoreOp storeop, VkImageLayout initiallayout, VkImageLayout finallayout)
{
	VkAttachmentDescription desc = {};

	desc.format			= format;
	desc.samples		= VK_SAMPLE_COUNT_1_BIT;			// TODO: multisample
	desc.loadOp			= loadop;
	desc.storeOp		= storeop;
	desc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// TODO: stencil load/store
	desc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	// TODO: stencil load/store
	desc.initialLayout	= initiallayout;
	desc.finalLayout	= finallayout;
	desc.flags			= 0;

	attachmentdescs.push_back(desc);
}

void VulkanRenderPass::AddDependency(uint32_t srcsubpass, VkAccessFlags srcaccess, VkPipelineStageFlags srcstage, uint32_t dstsubpass, VkAccessFlags dstaccess, VkPipelineStageFlags dststage, VkDependencyFlags flags)
{
	VkSubpassDependency dependency = {};

	dependency.srcSubpass		= srcsubpass;
	dependency.srcAccessMask	= srcaccess;
	dependency.srcStageMask		= srcstage;
	dependency.dstSubpass		= dstsubpass;
	dependency.dstAccessMask	= dstaccess;
	dependency.dstStageMask		= dststage;
	dependency.dependencyFlags	= flags;

	dependencies.push_back(dependency);
}

void VulkanRenderPass::AddSubpassInputReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout)
{
	VkAttachmentReference reference = {};

	reference.attachment = attachment;
	reference.layout = layout;

	inputreferences[subpass].push_back(reference);
}

void VulkanRenderPass::AddSubpassColorReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout)
{
	VkAttachmentReference reference = {};

	reference.attachment = attachment;
	reference.layout = layout;

	colorreferences[subpass].push_back(reference);
}

void VulkanRenderPass::AddSubpassPreserveReference(uint32_t subpass, uint32_t attachment)
{
	preservereferences[subpass].push_back(attachment);
}

void VulkanRenderPass::SetSubpassDepthReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout)
{
	depthreferences[subpass].attachment = attachment;
	depthreferences[subpass].layout = layout;
}

void VulkanRenderPass::Begin(VkCommandBuffer commandbuffer, VkFramebuffer framebuffer, VkSubpassContents contents, const Math::Color& clearcolor, float cleardepth, uint8_t clearstencil)
{
	for (size_t i = 0; i < attachmentdescs.size(); ++i) {
		bool isdepth = (
			attachmentdescs[i].initialLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
			attachmentdescs[i].initialLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		if (isdepth) {
			clearcolors[i].depthStencil.depth = cleardepth;
			clearcolors[i].depthStencil.stencil = clearstencil;
		} else {
			clearcolors[i].color.float32[0] = clearcolor.r;
			clearcolors[i].color.float32[1] = clearcolor.g;
			clearcolors[i].color.float32[2] = clearcolor.b;
			clearcolors[i].color.float32[3] = clearcolor.a;
		}
	}

	VkRenderPassBeginInfo passbegininfo	= {};

	passbegininfo.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passbegininfo.pNext						= NULL;
	passbegininfo.renderPass				= renderpass;
	passbegininfo.framebuffer				= framebuffer;
	passbegininfo.renderArea.offset.x		= 0;
	passbegininfo.renderArea.offset.y		= 0;
	passbegininfo.renderArea.extent.width	= framewidth;
	passbegininfo.renderArea.extent.height	= frameheight;
	passbegininfo.clearValueCount			= (uint32_t)attachmentdescs.size();
	passbegininfo.pClearValues				= clearcolors;

	vkCmdBeginRenderPass(commandbuffer, &passbegininfo, contents);
}

void VulkanRenderPass::End(VkCommandBuffer commandbuffer)
{
	vkCmdEndRenderPass(commandbuffer);
}

void VulkanRenderPass::NextSubpass(VkCommandBuffer commandbuffer, VkSubpassContents contents)
{
	vkCmdNextSubpass(commandbuffer, contents);
}

bool VulkanRenderPass::Assemble()
{
	VK_ASSERT(renderpass == nullptr);

	VkRenderPassCreateInfo	renderpassinfo	= {};
	VkSubpassDescription*	subpasses		= new VkSubpassDescription[numsubpasses];
	VkResult				res;
	size_t					depthid			= SIZE_MAX;

	// setup subpasses
	for (uint32_t i = 0; i < numsubpasses; ++i) {
		VkSubpassDescription&	subpass			= subpasses[i];
		ReferenceArray&			inputrefs		= inputreferences[i];
		ReferenceArray&			colorrefs		= colorreferences[i];
		PreserveArray&			preserverefs	= preservereferences[i];
		VkAttachmentReference&	depthref		= depthreferences[i];

		subpass.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags						= 0;
		subpass.inputAttachmentCount		= (uint32_t)inputrefs.size();
		subpass.pInputAttachments			= (inputrefs.size() == 0 ? NULL : inputrefs.data());
		subpass.colorAttachmentCount		= (uint32_t)colorrefs.size();
		subpass.pColorAttachments			= (colorrefs.size() == 0 ? NULL : colorrefs.data());
		subpass.pResolveAttachments			= NULL;	// TODO: MSAA
		subpass.pDepthStencilAttachment		= (depthref.layout == VK_IMAGE_LAYOUT_UNDEFINED ? NULL : &depthref);
		subpass.preserveAttachmentCount		= (uint32_t)preserverefs.size();
		subpass.pPreserveAttachments		= (preserverefs.size() == 0 ? NULL : preserverefs.data());
	}

	// create render pass
	renderpassinfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassinfo.pNext				= NULL;
	renderpassinfo.attachmentCount		= (uint32_t)attachmentdescs.size();
	renderpassinfo.pAttachments			= attachmentdescs.data();
	renderpassinfo.subpassCount			= numsubpasses;
	renderpassinfo.pSubpasses			= subpasses;
	renderpassinfo.dependencyCount		= (uint32_t)dependencies.size();
	renderpassinfo.pDependencies		= (dependencies.size() == 0 ? NULL : dependencies.data());

	res = vkCreateRenderPass(driverInfo.device, &renderpassinfo, NULL, &renderpass);

	if (res != VK_SUCCESS)
		return false;

	clearcolors = new VkClearValue[attachmentdescs.size()];
	delete[] subpasses;

	return true;
}

// --- VulkanSpecializationInfo impl ------------------------------------------

VulkanSpecializationInfo::VulkanSpecializationInfo()
{
	specinfo.mapEntryCount	= 0;
	specinfo.dataSize		= 0;
	specinfo.pData			= malloc(256);

	entries.reserve(16);
}

VulkanSpecializationInfo::VulkanSpecializationInfo(const VulkanSpecializationInfo& other)
{
	entries.reserve(16);

	specinfo	= other.specinfo;
	entries		= other.entries;

	specinfo.pData = malloc(256);
	memcpy((void*)specinfo.pData, other.specinfo.pData, specinfo.dataSize);
}

VulkanSpecializationInfo::~VulkanSpecializationInfo()
{
	free((void*)specinfo.pData);
}

void VulkanSpecializationInfo::AddInt(uint32_t constantID, int32_t value)
{
	VK_ASSERT(specinfo.dataSize + sizeof(value) < 256);

	VkSpecializationMapEntry entry;

	entry.constantID	= constantID;
	entry.offset		= (uint32_t)specinfo.dataSize;
	entry.size			= sizeof(value);

	*((int32_t*)((uint8_t*)specinfo.pData + entry.offset)) = value;
	specinfo.dataSize += entry.size;

	entries.push_back(entry);

	++specinfo.mapEntryCount;
	specinfo.pMapEntries = entries.data();
}

void VulkanSpecializationInfo::AddUInt(uint32_t constantID, uint32_t value)
{
	VK_ASSERT(specinfo.dataSize + sizeof(value) < 256);

	VkSpecializationMapEntry entry;

	entry.constantID	= constantID;
	entry.offset		= (uint32_t)specinfo.dataSize;
	entry.size			= sizeof(value);

	*((uint32_t*)((uint8_t*)specinfo.pData + entry.offset)) = value;
	specinfo.dataSize += entry.size;

	entries.push_back(entry);

	++specinfo.mapEntryCount;
	specinfo.pMapEntries = entries.data();
}

void VulkanSpecializationInfo::AddFloat(uint32_t constantID, float value)
{
	VK_ASSERT(specinfo.dataSize + sizeof(value) < 256);

	VkSpecializationMapEntry entry;

	entry.constantID	= constantID;
	entry.offset		= (uint32_t)specinfo.dataSize;
	entry.size			= sizeof(value);

	*((float*)((uint8_t*)specinfo.pData + entry.offset)) = value;
	specinfo.dataSize += entry.size;

	entries.push_back(entry);

	++specinfo.mapEntryCount;
	specinfo.pMapEntries = entries.data();
}

// --- VulkanBasePipeline impl ------------------------------------------------

VulkanBasePipeline::BaseTemporaryData::BaseTemporaryData()
{
	shaderstages.reserve(5);
	pushconstantranges.reserve(2);
	specinfos.reserve(5);
}

VulkanBasePipeline::DescriptorSetGroup::DescriptorSetGroup()
{
	descriptorpool = 0;
	descsetlayout = 0;
}

void VulkanBasePipeline::DescriptorSetGroup::Reset()
{
	descriptorpool = 0;
	descsetlayout = 0;

	bindings.clear();
	descriptorsets.clear();
	descbufferinfos.clear();
	descimageinfos.clear();
}

VulkanBasePipeline::VulkanBasePipeline()
{
	pipeline		= NULL;
	pipelinelayout	= NULL;
	basetempdata	= nullptr;
}

VulkanBasePipeline::~VulkanBasePipeline()
{
	if (basetempdata != nullptr)
		delete basetempdata;

	for (size_t i = 0; i < shadermodules.size(); ++i) {
		vkDestroyShaderModule(driverInfo.device, shadermodules[i], 0);
	}

	for (size_t i = 0; i < descsetgroups.size(); ++i) {
		vkDestroyDescriptorSetLayout(driverInfo.device, descsetgroups[i].descsetlayout, 0);
		
		// NOTE: you can't call this when VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT is not set
		//vkFreeDescriptorSets(driverInfo.device, descsetgroups[i].descriptorpool, descsetgroups[i].descriptorsets.size(), descsetgroups[i].descriptorsets.data());

		if (descsetgroups[i].descriptorpool)
			vkDestroyDescriptorPool(driverInfo.device, descsetgroups[i].descriptorpool, 0);
	}

	if (pipeline)
		vkDestroyPipeline(driverInfo.device, pipeline, 0);

	if (pipelinelayout)
		vkDestroyPipelineLayout(driverInfo.device, pipelinelayout, 0);
}

bool VulkanBasePipeline::AddShader(VkShaderStageFlagBits type, const char* file, const VulkanSpecializationInfo& specinfo)
{
	VkPipelineShaderStageCreateInfo	shaderstageinfo		= {};
	VkShaderModuleCreateInfo		modulecreateinfo	= {};
	SPIRVByteCode					spirvcode;
	FILE*							infile				= nullptr;
	std::string						ext;
	std::string						sourcefile(file);
	bool							isspirv				= false;

	Math::GetExtension(ext, file);
	isspirv = (ext[0] == 's' && ext[1] == 'p');

	if (isspirv) {
		// I don't bother with SPIR-V anymore
		VK_ASSERT(false);
		return false;
	}

#ifdef _MSC_VER
	fopen_s(&infile, sourcefile.c_str(), "rb");
#else
	infile = fopen(sourcefile.c_str(), "rb")
#endif

	if (infile == nullptr)
		return false;

	fseek(infile, 0, SEEK_END);
	long filelength = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	EShLanguage			stage		= FindLanguage(type);
	EShMessages			messages	= (EShMessages)(EShMsgSpvRules|EShMsgVulkanRules);
	glslang::TShader	shader(stage);
	glslang::TProgram	program;

	char* source = (char*)malloc(filelength + 1);

	fread(source, 1, filelength, infile);
	source[filelength] = 0;
	
	shader.setStrings(&source, 1);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

	if (!shader.parse(&SPIRVResources, 110, false, messages)) {
		printf("%s\n", shader.getInfoLog());
		printf("%s\n", shader.getInfoDebugLog());
		
		return false;
	}

	// translate
	program.addShader(&shader);

	if (!program.link(messages)) {
		printf("%s\n", program.getInfoLog());
		printf("%s\n", program.getInfoDebugLog());

		return false;
	}

	glslang::GlslangToSpv(*program.getIntermediate(stage), spirvcode);

	free(source);
	fclose(infile);

	// create shader module
	shaderstageinfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderstageinfo.pNext				= NULL;
	shaderstageinfo.pSpecializationInfo	= NULL;
	shaderstageinfo.flags				= 0;
	shaderstageinfo.stage				= type;
	shaderstageinfo.pName				= "main";

	modulecreateinfo.sType				= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	modulecreateinfo.pNext				= NULL;
	modulecreateinfo.flags				= 0;
	modulecreateinfo.codeSize			= spirvcode.size() * sizeof(uint32_t);
	modulecreateinfo.pCode				= spirvcode.data();

	VkResult res = vkCreateShaderModule(driverInfo.device, &modulecreateinfo, NULL, &shaderstageinfo.module);

	if (res != VK_SUCCESS)
		return false;

	basetempdata->specinfos.push_back(specinfo);
	basetempdata->shaderstages.push_back(shaderstageinfo);
	shadermodules.push_back(shaderstageinfo.module);

	return true;
}

void VulkanBasePipeline::SetDescriptorSetLayoutBufferBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
{
	if (currentgroup.bindings.size() <= binding)
		currentgroup.bindings.resize(binding + 1);

	if (currentgroup.descbufferinfos.size() <= binding)
		currentgroup.descbufferinfos.resize(binding + 1);

	currentgroup.bindings[binding].binding				= binding;
	currentgroup.bindings[binding].descriptorCount		= 1;	// NOTE: array size
	currentgroup.bindings[binding].descriptorType		= type;
	currentgroup.bindings[binding].pImmutableSamplers	= NULL;
	currentgroup.bindings[binding].stageFlags			= stage;
}

void VulkanBasePipeline::SetDescriptorSetLayoutImageBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
{
	if (currentgroup.bindings.size() <= binding)
		currentgroup.bindings.resize(binding + 1);

	if (currentgroup.descimageinfos.size() <= binding)
		currentgroup.descimageinfos.resize(binding + 1);

	currentgroup.bindings[binding].binding				= binding;
	currentgroup.bindings[binding].descriptorCount		= 1;	// NOTE: array size
	currentgroup.bindings[binding].descriptorType		= type;
	currentgroup.bindings[binding].pImmutableSamplers	= NULL;
	currentgroup.bindings[binding].stageFlags			= stage;
}

void VulkanBasePipeline::AllocateDescriptorSets(uint32_t numsets)
{
	// setup bindings first to define layout, then call this function
	VK_ASSERT(currentgroup.bindings.size() > 0);

	VkDescriptorSetLayoutCreateInfo	descsetlayoutinfo	= {};
	VkDescriptorPoolCreateInfo		descpoolinfo		= {};
	VkDescriptorSetAllocateInfo		descsetallocinfo	= {};
	VkResult						res;

	// descriptor set layout
	descsetlayoutinfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descsetlayoutinfo.pNext			= NULL;
	descsetlayoutinfo.bindingCount	= (uint32_t)currentgroup.bindings.size();
	descsetlayoutinfo.pBindings		= currentgroup.bindings.data();

	res = vkCreateDescriptorSetLayout(driverInfo.device, &descsetlayoutinfo, 0, &currentgroup.descsetlayout);
	VK_ASSERT(res == VK_SUCCESS);

	// calculate number of descriptors for each type
	uint32_t numdescriptors[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	uint32_t numnonemptytypes = 0;
	uint32_t typeswritten = 0;

	memset(numdescriptors, 0, sizeof(numdescriptors));

	for (size_t i = 0; i < currentgroup.bindings.size(); ++i) {
		const VkDescriptorSetLayoutBinding& layoutbinding = currentgroup.bindings[i];
		numdescriptors[layoutbinding.descriptorType] += layoutbinding.descriptorCount;
	}

	for (size_t i = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
		if (numdescriptors[i] > 0)
			++numnonemptytypes;
	}

	// create descriptor pool
	VkDescriptorPoolSize* poolsizes = new VkDescriptorPoolSize[numnonemptytypes];

	for (size_t i = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
		if (numdescriptors[i] > 0) {
			poolsizes[typeswritten].type = (VkDescriptorType)i;
			poolsizes[typeswritten].descriptorCount = numdescriptors[i] * numsets;

			++typeswritten;
		}
	}

	descpoolinfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descpoolinfo.pNext			= NULL;
	descpoolinfo.maxSets		= numsets;
	descpoolinfo.poolSizeCount	= numnonemptytypes;
	descpoolinfo.pPoolSizes		= poolsizes;

	res = vkCreateDescriptorPool(driverInfo.device, &descpoolinfo, NULL, &currentgroup.descriptorpool);
	VK_ASSERT(res == VK_SUCCESS);

	delete[] poolsizes;

	// descriptor sets for this group
	VkDescriptorSetLayout* descsetlayouts = new VkDescriptorSetLayout[numsets];

	for (uint32_t i = 0; i < numsets; ++i)
		descsetlayouts[i] = currentgroup.descsetlayout;

	descsetallocinfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descsetallocinfo.pNext				= NULL;
	descsetallocinfo.descriptorPool		= currentgroup.descriptorpool;
	descsetallocinfo.descriptorSetCount	= numsets;
	descsetallocinfo.pSetLayouts		= descsetlayouts;

	currentgroup.descriptorsets.resize(numsets);

	res = vkAllocateDescriptorSets(driverInfo.device, &descsetallocinfo, currentgroup.descriptorsets.data());
	VK_ASSERT(res == VK_SUCCESS);

	delete[] descsetlayouts;

	descsetgroups.push_back(currentgroup);
	currentgroup.Reset();
}

void VulkanBasePipeline::SetDescriptorSetGroupBufferInfo(uint32_t group, uint32_t binding, const VkDescriptorBufferInfo* info)
{
	VK_ASSERT(group < descsetgroups.size());
	VK_ASSERT(info != nullptr);

	descsetgroups[group].descbufferinfos[binding] = *info;
}

void VulkanBasePipeline::SetDescriptorSetGroupImageInfo(uint32_t group, uint32_t binding, const VkDescriptorImageInfo* info)
{
	VK_ASSERT(group < descsetgroups.size());
	VK_ASSERT(info != nullptr);

	descsetgroups[group].descimageinfos[binding] = *info;
}

void VulkanBasePipeline::UpdateDescriptorSet(uint32_t group, uint32_t set)
{
	DescriptorSetGroup& dsgroup = descsetgroups[group];
	VkWriteDescriptorSet* descsetwrites = new VkWriteDescriptorSet[dsgroup.bindings.size()];

	for (size_t i = 0; i < dsgroup.bindings.size(); ++i) {
		VK_ASSERT(i == dsgroup.bindings[i].binding);

		descsetwrites[i].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descsetwrites[i].pNext				= NULL;
		descsetwrites[i].dstSet				= dsgroup.descriptorsets[set];
		descsetwrites[i].descriptorCount	= dsgroup.bindings[i].descriptorCount;
		descsetwrites[i].descriptorType		= dsgroup.bindings[i].descriptorType;
		descsetwrites[i].dstArrayElement	= 0;
		descsetwrites[i].dstBinding			= dsgroup.bindings[i].binding;

		descsetwrites[i].pBufferInfo		= 0;
		descsetwrites[i].pImageInfo			= 0;
		descsetwrites[i].pTexelBufferView	= 0;

		if (descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			VK_ASSERT(dsgroup.descbufferinfos[i].buffer != NULL);
			descsetwrites[i].pBufferInfo = &dsgroup.descbufferinfos[i];
		}

		if (descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
			descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			VK_ASSERT(dsgroup.descimageinfos[i].imageView != NULL);
			VK_ASSERT(dsgroup.descimageinfos[i].imageLayout != VK_IMAGE_LAYOUT_UNDEFINED);

			descsetwrites[i].pImageInfo = &dsgroup.descimageinfos[i];
		}

		if (descsetwrites[i].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
			VK_ASSERT(dsgroup.descimageinfos[i].imageView != NULL);
			descsetwrites[i].pImageInfo = &dsgroup.descimageinfos[i];
		}

		// TODO: other types
	}

	vkUpdateDescriptorSets(driverInfo.device, (uint32_t)dsgroup.bindings.size(), descsetwrites, 0, NULL);
	delete[] descsetwrites;
}

void VulkanBasePipeline::AddPushConstantRange(VkShaderStageFlags stages, uint32_t offset, uint32_t size)
{
	VK_ASSERT(offset % 4 == 0);
	VK_ASSERT(size % 4 == 0);

	VkPushConstantRange range;

	range.offset		= offset;
	range.size			= size;
	range.stageFlags	= stages;

	basetempdata->pushconstantranges.push_back(range);
}

bool VulkanBasePipeline::Assemble()
{
	VkPipelineLayoutCreateInfo	layoutinfo = {};
	VkResult					res;

	// collect layouts
	VkDescriptorSetLayout* descsetlayouts = new VkDescriptorSetLayout[descsetgroups.size()];

	for (size_t i = 0; i < descsetgroups.size(); ++i)
		descsetlayouts[i] = descsetgroups[i].descsetlayout;

	// pipeline layout
	layoutinfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutinfo.pNext					= NULL;
	layoutinfo.pushConstantRangeCount	= (uint32_t)basetempdata->pushconstantranges.size();
	layoutinfo.pPushConstantRanges		= basetempdata->pushconstantranges.data();
	layoutinfo.setLayoutCount			= (uint32_t)descsetgroups.size();
	layoutinfo.pSetLayouts				= descsetlayouts;

	res = vkCreatePipelineLayout(driverInfo.device, &layoutinfo, NULL, &pipelinelayout);
	delete[] descsetlayouts;

	return (res == VK_SUCCESS);
}

// --- VulkanGraphicsPipeline impl --------------------------------------------

VulkanGraphicsPipeline::GraphicsTemporaryData::GraphicsTemporaryData()
{
	attributes.reserve(8);

	// vertex input
	vertexinputstate.sType						= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexinputstate.pNext						= NULL;
	vertexinputstate.flags						= 0;

	// input assembler
	inputassemblystate.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputassemblystate.pNext					= NULL;
	inputassemblystate.flags					= 0;
	inputassemblystate.primitiveRestartEnable	= VK_FALSE;
	inputassemblystate.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// rasterizer state
	rasterizationstate.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationstate.pNext					= NULL;
	rasterizationstate.flags					= 0;
	rasterizationstate.polygonMode				= VK_POLYGON_MODE_FILL;
	rasterizationstate.cullMode					= VK_CULL_MODE_BACK_BIT;
	rasterizationstate.frontFace				= VK_FRONT_FACE_COUNTER_CLOCKWISE;	// as in OpenGL
	rasterizationstate.depthClampEnable			= VK_FALSE;
	rasterizationstate.rasterizerDiscardEnable	= VK_FALSE;
	rasterizationstate.depthBiasEnable			= VK_FALSE;
	rasterizationstate.depthBiasConstantFactor	= 0;
	rasterizationstate.depthBiasClamp			= 0;
	rasterizationstate.depthBiasSlopeFactor		= 0;
	rasterizationstate.lineWidth				= 1;

	// blend state
	blendstates.resize(1);

	blendstates[0].colorWriteMask				= 0xf;
	blendstates[0].blendEnable					= VK_FALSE;
	blendstates[0].alphaBlendOp					= VK_BLEND_OP_ADD;
	blendstates[0].colorBlendOp					= VK_BLEND_OP_ADD;
	blendstates[0].srcColorBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendstates[0].dstColorBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendstates[0].srcAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendstates[0].dstAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO;

	colorblendstate.sType						= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblendstate.flags						= 0;
	colorblendstate.pNext						= NULL;
	colorblendstate.attachmentCount				= 1;
	colorblendstate.pAttachments				= blendstates.data();
	colorblendstate.logicOpEnable				= VK_FALSE;
	colorblendstate.logicOp						= VK_LOGIC_OP_NO_OP;
	colorblendstate.blendConstants[0]			= 1.0f;
	colorblendstate.blendConstants[1]			= 1.0f;
	colorblendstate.blendConstants[2]			= 1.0f;
	colorblendstate.blendConstants[3]			= 1.0f;

	// viewport state
	viewportstate.sType							= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportstate.pNext							= NULL;
	viewportstate.flags							= 0;
	viewportstate.viewportCount					= 1;
	viewportstate.scissorCount					= 1;
	viewportstate.pScissors						= NULL;
	viewportstate.pViewports					= NULL;

	// depth-stencil state
	depthstencilstate.sType						= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthstencilstate.pNext						= NULL;
	depthstencilstate.flags						= 0;
	depthstencilstate.depthTestEnable			= VK_FALSE;
	depthstencilstate.depthWriteEnable			= VK_FALSE;
	depthstencilstate.depthCompareOp			= VK_COMPARE_OP_LESS_OR_EQUAL;
	depthstencilstate.depthBoundsTestEnable		= VK_FALSE;
	depthstencilstate.stencilTestEnable			= VK_FALSE;
	depthstencilstate.back.failOp				= VK_STENCIL_OP_KEEP;
	depthstencilstate.back.passOp				= VK_STENCIL_OP_KEEP;
	depthstencilstate.back.depthFailOp			= VK_STENCIL_OP_KEEP;
	depthstencilstate.back.compareOp			= VK_COMPARE_OP_ALWAYS;
	depthstencilstate.back.compareMask			= 0;
	depthstencilstate.back.reference			= 0;
	depthstencilstate.back.writeMask			= 0;
	depthstencilstate.minDepthBounds			= 0;
	depthstencilstate.maxDepthBounds			= 0;
	depthstencilstate.front						= depthstencilstate.back;

	// multisample state
	multisamplestate.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplestate.pNext						= NULL;
	multisamplestate.flags						= 0;
	multisamplestate.pSampleMask				= NULL;
	multisamplestate.rasterizationSamples		= VK_SAMPLE_COUNT_1_BIT;
	multisamplestate.sampleShadingEnable		= VK_FALSE;
	multisamplestate.alphaToCoverageEnable		= VK_FALSE;
	multisamplestate.alphaToOneEnable			= VK_FALSE;
	multisamplestate.minSampleShading			= 0.0f;
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline()
{
	tempdata				= new GraphicsTemporaryData();
	basetempdata			= tempdata;

	viewport.x				= 0;
	viewport.y				= 0;
	viewport.width			= 800;
	viewport.height			= 600;
	viewport.minDepth		= 0;
	viewport.maxDepth		= 1;

	scissor.extent.width	= 800;
	scissor.extent.height	= 600;
	scissor.offset.x		= 0;
	scissor.offset.y		= 0;

	tempdata->viewportstate.pScissors = &scissor;
	tempdata->viewportstate.pViewports = &viewport;
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
}

bool VulkanGraphicsPipeline::Assemble(VkRenderPass renderpass, uint32_t subpass)
{
	VK_ASSERT(tempdata != NULL);
	VK_ASSERT(VulkanBasePipeline::Assemble());

	VkGraphicsPipelineCreateInfo	pipelineinfo = {};
	VkDynamicState					dynstateenables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkResult						res;

	memset(dynstateenables, 0, sizeof(dynstateenables));
	tempdata->dynamicstate.dynamicStateCount	= 0;

	dynstateenables[0] = VK_DYNAMIC_STATE_VIEWPORT;
	++tempdata->dynamicstate.dynamicStateCount;

	dynstateenables[1] = VK_DYNAMIC_STATE_SCISSOR;
	++tempdata->dynamicstate.dynamicStateCount;

	tempdata->dynamicstate.sType			= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	tempdata->dynamicstate.pNext			= NULL;
	tempdata->dynamicstate.pDynamicStates	= dynstateenables;
	tempdata->dynamicstate.flags			= 0;

	// input state
	tempdata->vertexinputstate.vertexBindingDescriptionCount	= (uint32_t)tempdata->vertexbindings.size();
	tempdata->vertexinputstate.pVertexBindingDescriptions		= tempdata->vertexbindings.data();
	tempdata->vertexinputstate.vertexAttributeDescriptionCount	= (uint32_t)tempdata->attributes.size();
	tempdata->vertexinputstate.pVertexAttributeDescriptions		= tempdata->attributes.data();

	for (size_t i = 0; i < tempdata->shaderstages.size(); ++i) {
		if (tempdata->specinfos[i].GetSpecializationInfo()->dataSize > 0)
			tempdata->shaderstages[i].pSpecializationInfo = tempdata->specinfos[i].GetSpecializationInfo();
	}

	// pipeline
	pipelineinfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineinfo.pNext					= NULL;
	pipelineinfo.layout					= pipelinelayout;
	pipelineinfo.basePipelineHandle		= VK_NULL_HANDLE;
	pipelineinfo.basePipelineIndex		= 0;
	pipelineinfo.flags					= 0;
	pipelineinfo.pVertexInputState		= &tempdata->vertexinputstate;
	pipelineinfo.pInputAssemblyState	= &tempdata->inputassemblystate;
	pipelineinfo.pRasterizationState	= &tempdata->rasterizationstate;
	pipelineinfo.pColorBlendState		= &tempdata->colorblendstate;
	pipelineinfo.pTessellationState		= NULL;
	pipelineinfo.pMultisampleState		= &tempdata->multisamplestate;
	pipelineinfo.pDynamicState			= &tempdata->dynamicstate;
	pipelineinfo.pViewportState			= &tempdata->viewportstate;
	pipelineinfo.pDepthStencilState		= &tempdata->depthstencilstate;
	pipelineinfo.pStages				= tempdata->shaderstages.data();
	pipelineinfo.stageCount				= (uint32_t)tempdata->shaderstages.size();
	pipelineinfo.renderPass				= renderpass;
	pipelineinfo.subpass				= subpass;

	res = vkCreateGraphicsPipelines(driverInfo.device, VK_NULL_HANDLE, 1, &pipelineinfo, NULL, &pipeline);
	delete tempdata;

	tempdata = nullptr;
	basetempdata = nullptr;

	return (res == VK_SUCCESS);
}

void VulkanGraphicsPipeline::SetInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
{
	if (tempdata->attributes.size() <= location)
		tempdata->attributes.resize(location + 1);

	VkVertexInputAttributeDescription& attribdesc = tempdata->attributes[location];

	attribdesc.location	= location;
	attribdesc.binding	= binding;
	attribdesc.format	= format;
	attribdesc.offset	= offset;
}

void VulkanGraphicsPipeline::SetVertexInputBinding(uint32_t location, const VkVertexInputBindingDescription& desc)
{
	VkVertexInputBindingDescription defvalue;

	defvalue.binding	= 0;
	defvalue.inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;
	defvalue.stride		= 0;

	if (tempdata->vertexbindings.size() <= location)
		tempdata->vertexbindings.resize(location + 1, defvalue);

	tempdata->vertexbindings[location] = desc;
}

void VulkanGraphicsPipeline::SetInputAssemblerState(VkPrimitiveTopology topology, VkBool32 primitiverestart)
{
	tempdata->inputassemblystate.topology = topology;
	tempdata->inputassemblystate.primitiveRestartEnable = primitiverestart;
}

void VulkanGraphicsPipeline::SetRasterizerState(VkPolygonMode fillmode, VkCullModeFlags cullmode)
{
	tempdata->rasterizationstate.polygonMode = fillmode;
	tempdata->rasterizationstate.cullMode = cullmode;
}

void VulkanGraphicsPipeline::SetDepthState(VkBool32 depthenable, VkBool32 depthwriteenable, VkCompareOp compareop)
{
	tempdata->depthstencilstate.depthTestEnable = depthenable;
	tempdata->depthstencilstate.depthWriteEnable = depthwriteenable;
	tempdata->depthstencilstate.depthCompareOp = compareop;
}

void VulkanGraphicsPipeline::SetBlendState(uint32_t attachment, VkBool32 enable, VkBlendOp colorop, VkBlendFactor srcblend, VkBlendFactor destblend)
{
	if (attachment >= tempdata->blendstates.size())
		tempdata->blendstates.resize(attachment + 1);

	VkPipelineColorBlendAttachmentState& blendstate = tempdata->blendstates[attachment];

	blendstate.colorWriteMask		= 0xf;
	blendstate.blendEnable			= enable;
	blendstate.alphaBlendOp			= VK_BLEND_OP_ADD;
	blendstate.colorBlendOp			= colorop;
	blendstate.srcColorBlendFactor	= srcblend;
	blendstate.dstColorBlendFactor	= destblend;
	blendstate.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
	blendstate.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;

	tempdata->colorblendstate.attachmentCount = (uint32_t)tempdata->blendstates.size();
	tempdata->colorblendstate.pAttachments = tempdata->blendstates.data();
}

void VulkanGraphicsPipeline::SetViewport(float x, float y, float width, float height, float minz, float maxz)
{
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minz;
	viewport.maxDepth = maxz;
}

void VulkanGraphicsPipeline::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = x;
	scissor.offset.y = y;
}

// --- VulkanComputePipeline impl ---------------------------------------------

VulkanComputePipeline::VulkanComputePipeline()
{
	basetempdata = new BaseTemporaryData();
}

VulkanComputePipeline::~VulkanComputePipeline()
{
}

bool VulkanComputePipeline::Assemble()
{
	BaseTemporaryData* tempdata = basetempdata;

	VK_ASSERT(tempdata != NULL);
	VK_ASSERT(tempdata->shaderstages.size() == 1);
	VK_ASSERT(tempdata->shaderstages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);

	VK_ASSERT(VulkanBasePipeline::Assemble());

	VkComputePipelineCreateInfo	pipelineinfo = {};
	VkResult					res;

	if (tempdata->specinfos[0].GetSpecializationInfo()->dataSize > 0)
		tempdata->shaderstages[0].pSpecializationInfo = tempdata->specinfos[0].GetSpecializationInfo();

	pipelineinfo.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineinfo.pNext					= NULL;
	pipelineinfo.layout					= pipelinelayout;
	pipelineinfo.basePipelineHandle		= VK_NULL_HANDLE;
	pipelineinfo.basePipelineIndex		= 0;
	pipelineinfo.flags					= 0;
	pipelineinfo.stage					= tempdata->shaderstages[0];

	res = vkCreateComputePipelines(driverInfo.device, VK_NULL_HANDLE, 1, &pipelineinfo, NULL, &pipeline);
	delete tempdata;

	tempdata = nullptr;
	basetempdata = nullptr;

	return (res == VK_SUCCESS);
}

// --- VulkanPipelineBarrierBatch impl ----------------------------------------

VulkanPipelineBarrierBatch::VulkanPipelineBarrierBatch(VkPipelineStageFlags srcstage, VkPipelineStageFlags dststage)
{
	src = srcstage;
	dst = dststage;

	buffbarriers.reserve(4);
	imgbarriers.reserve(4);
}

void VulkanPipelineBarrierBatch::BufferAccessBarrier(VkBuffer buffer, VkAccessFlags srcaccess, VkAccessFlags dstaccess, VkDeviceSize offset, VkDeviceSize size)
{
	VkBufferMemoryBarrier barrier = {};

	barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext				= NULL;
	barrier.srcAccessMask		= srcaccess;
	barrier.dstAccessMask		= dstaccess;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer				= buffer;
	barrier.offset				= offset;
	barrier.size				= size;

	buffbarriers.push_back(barrier);
}

void VulkanPipelineBarrierBatch::ImageLayoutTransition(VkImage image, VkAccessFlags srcaccess, VkAccessFlags dstaccess, VkImageAspectFlags aspectmask, VkImageLayout oldlayout, VkImageLayout newlayout, uint32_t firstmip, uint32_t mipcount, uint32_t firstslice, uint32_t slicecount)
{
	VkImageMemoryBarrier barrier = {};
	
	// don't know it
	VK_ASSERT(mipcount != 0);
	VK_ASSERT(slicecount != 0);

	barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext							= NULL;
	barrier.srcAccessMask					= srcaccess;
	barrier.dstAccessMask					= dstaccess;
	barrier.oldLayout						= oldlayout;
	barrier.newLayout						= newlayout;
	barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.image							= image;
	barrier.subresourceRange.aspectMask		= aspectmask;
	barrier.subresourceRange.baseMipLevel	= firstmip;
	barrier.subresourceRange.levelCount		= mipcount;
	barrier.subresourceRange.baseArrayLayer	= firstslice;
	barrier.subresourceRange.layerCount		= slicecount;

	imgbarriers.push_back(barrier);
}

void VulkanPipelineBarrierBatch::ImageLayoutTransition(VulkanImage* image, VkAccessFlags dstaccess, VkImageAspectFlags aspectmask, VkImageLayout newlayout, uint32_t firstmip, uint32_t mipcount, uint32_t firstslice, uint32_t slicecount)
{
	VkImageMemoryBarrier barrier = {};

	if (mipcount == 0)
		mipcount = image->GetMipMapCount();

	if (slicecount == 0)
		slicecount = image->GetArraySize();

	// TODO: check if access flags and layouts match in the subresource range

	barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext							= NULL;
	barrier.srcAccessMask					= image->GetAccess(firstmip, firstslice);
	barrier.dstAccessMask					= dstaccess;
	barrier.oldLayout						= image->GetLayout(firstmip, firstslice);
	barrier.newLayout						= newlayout;
	barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.image							= image->GetImage();
	barrier.subresourceRange.aspectMask		= aspectmask;
	barrier.subresourceRange.baseMipLevel	= firstmip;
	barrier.subresourceRange.levelCount		= mipcount;
	barrier.subresourceRange.baseArrayLayer	= firstslice;
	barrier.subresourceRange.layerCount		= slicecount;

	// I don't have any better idea...
	for (uint32_t i = firstmip; i < firstmip + mipcount; ++i) {
		for (uint32_t j = firstslice; j < firstslice + slicecount; ++j) {
			image->accesses[i * image->GetArraySize() + j] = dstaccess;
			image->layouts[i * image->GetArraySize() + j] = newlayout;
		}
	}

	// NOTE: this is the expected layout; don't use it before creating the descriptor!!!
	image->imageinfo.imageLayout = newlayout;

	imgbarriers.push_back(barrier);
}

void VulkanPipelineBarrierBatch::Enlist(VkCommandBuffer commandbuffer)
{
	VkImageMemoryBarrier*	ibarriers = NULL;
	VkBufferMemoryBarrier*	bbarriers = NULL;

	if (buffbarriers.size() > 0)
		bbarriers = buffbarriers.data();

	if (imgbarriers.size() > 0)
		ibarriers = imgbarriers.data();

	if (bbarriers || ibarriers)
		vkCmdPipelineBarrier(commandbuffer, src, dst, 0, 0, NULL, (uint32_t)buffbarriers.size(), bbarriers, (uint32_t)imgbarriers.size(), ibarriers);

	buffbarriers.clear();
	imgbarriers.clear();
}

void VulkanPipelineBarrierBatch::Reset()
{
	buffbarriers.clear();
	imgbarriers.clear();
}

void VulkanPipelineBarrierBatch::Reset(VkPipelineStageFlags srcstage, VkPipelineStageFlags dststage)
{
	src = srcstage;
	dst = dststage;

	buffbarriers.clear();
	imgbarriers.clear();
}

// --- VulkanScreenQuad impl --------------------------------------------------

VulkanScreenQuad::VulkanScreenQuad(VkRenderPass renderpass, uint32_t subpass, VulkanImage* image, const char* psfile)
{
	Math::MatrixIdentity(transform);

	pipeline = new VulkanGraphicsPipeline();

	pipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/screenquad.vert");
	pipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, psfile);

	pipeline->SetDescriptorSetLayoutImageBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	pipeline->AllocateDescriptorSets(1);
	pipeline->AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, 64);

	pipeline->SetDescriptorSetGroupImageInfo(0, 0, image->GetImageInfo());
	pipeline->UpdateDescriptorSet(0, 0);

	pipeline->SetInputAssemblerState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
	pipeline->SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
	pipeline->SetBlendState(0, VK_TRUE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);

	VK_ASSERT(pipeline->Assemble(renderpass, subpass));
}

VulkanScreenQuad::~VulkanScreenQuad()
{
	delete pipeline;
}

void VulkanScreenQuad::Draw(VkCommandBuffer commandbuffer)
{
	vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
	vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, pipeline->GetDescriptorSets(0), 0, NULL);
	vkCmdPushConstants(commandbuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &transform);

	vkCmdSetViewport(commandbuffer, 0, 1, pipeline->GetViewport());
	vkCmdSetScissor(commandbuffer, 0, 1, pipeline->GetScissor());

	vkCmdDraw(commandbuffer, 4, 1, 0, 0);
}

void VulkanScreenQuad::SetTextureMatrix(const Math::Matrix& m)
{
	transform = m;
}

// --- VulkanPresentationEngine impl ------------------------------------------

VulkanPresentationEngine::VulkanPresentationEngine(uint32_t width, uint32_t height)
{
	renderpass		= nullptr;
	framebuffers	= nullptr;
	depthbuffer		= nullptr;

	buffersinflight	= 0;
	currentframe	= 0;
	currentdrawable	= 0;

	// setup frame queueing
	VkCommandBufferAllocateInfo	cmdbuffinfo	= {};
	VkSemaphoreCreateInfo		semainfo	= {};
	VkFenceCreateInfo			fenceinfo	= {};
	VkResult					res;
	
	semainfo.sType					= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semainfo.pNext					= NULL;
	semainfo.flags					= 0;

	fenceinfo.sType					= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceinfo.pNext					= NULL;
	fenceinfo.flags					= 0;

	cmdbuffinfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbuffinfo.pNext				= NULL;
	cmdbuffinfo.commandPool			= driverInfo.commandPool;
	cmdbuffinfo.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbuffinfo.commandBufferCount	= NUM_QUEUED_FRAMES;

	res = vkAllocateCommandBuffers(driverInfo.device, &cmdbuffinfo, commandbuffers);
	VK_ASSERT(res == VK_SUCCESS);

	for (int i = 0; i < NUM_QUEUED_FRAMES; ++i) {
		res = vkCreateSemaphore(driverInfo.device, &semainfo, NULL, &acquiresemas[i]);
		VK_ASSERT(res == VK_SUCCESS);

		res = vkCreateSemaphore(driverInfo.device, &semainfo, NULL, &presentsemas[i]);
		VK_ASSERT(res == VK_SUCCESS);

		res = vkCreateFence(driverInfo.device, &fenceinfo, NULL, &fences[i]);
		VK_ASSERT(res == VK_SUCCESS);
	}

	// create default depth buffer, framebuffers and render pass
	VkFramebufferCreateInfo		framebufferinfo = {};
	VkAttachmentReference		colorreference	= {};
	VkAttachmentReference		depthreference	= {};
	VkRenderPassCreateInfo		renderpassinfo	= {};
	VkSubpassDescription		subpass			= {};
	VkAttachmentDescription		rpattachments[2];
	VkImageView					fbattachments[2];
	VkFormat					depthformat		= VK_FORMAT_D24_UNORM_S8_UINT;

	if (QueryFormatSupport(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		depthformat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	depthbuffer = VulkanImage::Create2D(depthformat, width, height, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VK_ASSERT(depthbuffer != nullptr);

	// render pass
	rpattachments[0].format			= driverInfo.format;
	rpattachments[0].samples		= VK_SAMPLE_COUNT_1_BIT;
	rpattachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	rpattachments[0].storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
	rpattachments[0].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	rpattachments[0].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	rpattachments[0].initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	rpattachments[0].finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	rpattachments[0].flags			= 0;

	rpattachments[1].format			= depthbuffer->GetFormat();
	rpattachments[1].samples		= VK_SAMPLE_COUNT_1_BIT;
	rpattachments[1].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	rpattachments[1].storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	rpattachments[1].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
	rpattachments[1].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	rpattachments[1].initialLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	rpattachments[1].finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	rpattachments[1].flags			= 0;

	colorreference.attachment		= 0;
	colorreference.layout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depthreference.attachment		= 1;
	depthreference.layout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags					= 0;
	subpass.inputAttachmentCount	= 0;
	subpass.pInputAttachments		= NULL;
	subpass.colorAttachmentCount	= 1;
	subpass.pColorAttachments		= &colorreference;
	subpass.pResolveAttachments		= NULL;
	subpass.pDepthStencilAttachment	= &depthreference;
	subpass.preserveAttachmentCount	= 0;
	subpass.pPreserveAttachments	= NULL;

	renderpassinfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassinfo.pNext			= NULL;
	renderpassinfo.attachmentCount	= 2;
	renderpassinfo.pAttachments		= rpattachments;
	renderpassinfo.subpassCount		= 1;
	renderpassinfo.pSubpasses		= &subpass;
	renderpassinfo.dependencyCount	= 0;
	renderpassinfo.pDependencies	= NULL;

	res = vkCreateRenderPass(driverInfo.device, &renderpassinfo, NULL, &renderpass);
	VK_ASSERT(res == VK_SUCCESS);

	// framebuffers
	framebuffers = new VkFramebuffer[driverInfo.numSwapChainImages];

	framebufferinfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferinfo.pNext			= NULL;
	framebufferinfo.renderPass		= renderpass;
	framebufferinfo.attachmentCount	= 2;
	framebufferinfo.pAttachments	= fbattachments;
	framebufferinfo.width			= width;	//driverInfo.surfaceCaps.currentExtent.width;
	framebufferinfo.height			= height;	//driverInfo.surfaceCaps.currentExtent.height;
	framebufferinfo.layers			= 1;

	fbattachments[1] = depthbuffer->GetImageView();

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		fbattachments[0] = driverInfo.swapchainImageViews[i];

		res = vkCreateFramebuffer(driverInfo.device, &framebufferinfo, NULL, &framebuffers[i]);
		VK_ASSERT(res == VK_SUCCESS);
	}
}

VulkanPresentationEngine::~VulkanPresentationEngine()
{
	vkDeviceWaitIdle(driverInfo.device);

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		if (framebuffers[i])
			vkDestroyFramebuffer(driverInfo.device, framebuffers[i], 0);
	}

	delete[] framebuffers;

	if (renderpass)
		vkDestroyRenderPass(driverInfo.device, renderpass, 0);

	if (depthbuffer != nullptr)
		depthbuffer->Release();

	for (int i = 0; i < NUM_QUEUED_FRAMES; ++i) {
		if (acquiresemas[i])
			vkDestroySemaphore(driverInfo.device, acquiresemas[i], NULL);

		if (presentsemas[i])
			vkDestroySemaphore(driverInfo.device, presentsemas[i], NULL);

		if (fences[i])
			vkDestroyFence(driverInfo.device, fences[i], 0);
	}

	vkFreeCommandBuffers(driverInfo.device, driverInfo.commandPool, NUM_QUEUED_FRAMES, commandbuffers);
}

bool VulkanPresentationEngine::QueryFormatSupport(VkFormat format, VkFormatFeatureFlags features)
{
	VkFormatProperties formatprops;
	vkGetPhysicalDeviceFormatProperties(driverInfo.selectedDevice, format, &formatprops);

	return ((formatprops.optimalTilingFeatures & features) == features);
}

uint32_t VulkanPresentationEngine::GetNextDrawable()
{
	VkResult res;

	res = vkAcquireNextImageKHR(driverInfo.device, driverInfo.swapchain, UINT64_MAX, acquiresemas[currentframe], NULL, &currentdrawable);
	VK_ASSERT(res == VK_SUCCESS);

	return currentdrawable;
}

void VulkanPresentationEngine::Present()
{
	VkImageMemoryBarrier	presentbarrier	= {};
	VkSubmitInfo			submitinfo		= {};
	VkPresentInfoKHR		presentinfo		= {};
	VkPipelineStageFlags	pipestageflags	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkResult				res;
	uint32_t				nextframe		= (currentframe + 1) % NUM_QUEUED_FRAMES;

	// pre-present barrier
	presentbarrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	presentbarrier.pNext							= NULL;
	presentbarrier.srcAccessMask					= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentbarrier.dstAccessMask					= VK_ACCESS_MEMORY_READ_BIT;
	presentbarrier.oldLayout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentbarrier.newLayout						= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentbarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	presentbarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	presentbarrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	presentbarrier.subresourceRange.baseMipLevel	= 0;
	presentbarrier.subresourceRange.levelCount		= 1;
	presentbarrier.subresourceRange.baseArrayLayer	= 0;
	presentbarrier.subresourceRange.layerCount		= 1;
	presentbarrier.image							= driverInfo.swapchainImages[currentframe];

	vkCmdPipelineBarrier(commandbuffers[currentframe], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &presentbarrier);
	vkEndCommandBuffer(commandbuffers[currentframe]);

	// submit command buffer
	submitinfo.pNext				= NULL;
	submitinfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitinfo.waitSemaphoreCount	= 1;
	submitinfo.pWaitSemaphores		= &acquiresemas[currentframe];
	submitinfo.pWaitDstStageMask	= &pipestageflags;
	submitinfo.commandBufferCount	= 1;
	submitinfo.pCommandBuffers		= &commandbuffers[currentframe];
	submitinfo.signalSemaphoreCount	= 1;
	submitinfo.pSignalSemaphores	= &presentsemas[currentframe];

	res = vkQueueSubmit(driverInfo.graphicsQueue, 1, &submitinfo, fences[currentframe]);
	VK_ASSERT(res == VK_SUCCESS);

	presentinfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentinfo.pNext				= NULL;
	presentinfo.swapchainCount		= 1;
	presentinfo.pSwapchains			= &driverInfo.swapchain;
	presentinfo.pImageIndices		= &currentdrawable;
	presentinfo.waitSemaphoreCount	= 1;
	presentinfo.pWaitSemaphores		= &presentsemas[currentframe];
	presentinfo.pResults			= NULL;

	res = vkQueuePresentKHR(driverInfo.graphicsQueue, &presentinfo);
	VK_ASSERT(res == VK_SUCCESS);

	++buffersinflight;

	if (buffersinflight == NUM_QUEUED_FRAMES) {
		do {
			res = vkWaitForFences(driverInfo.device, 1, &fences[nextframe], VK_TRUE, 100000000);
		} while (res == VK_TIMEOUT);

		vkResetFences(driverInfo.device, 1, &fences[nextframe]);
		vkResetCommandBuffer(commandbuffers[nextframe], 0);

		if (FrameFinished != nullptr)
			FrameFinished(nextframe);

		--buffersinflight;
	}

	VK_ASSERT(buffersinflight < NUM_QUEUED_FRAMES);
	currentframe = nextframe;
}

// --- Functions impl ---------------------------------------------------------

void VulkanRenderText(const std::string& str, VulkanImage* image)
{
#ifdef _WIN32
	VulkanRenderTextEx(str, image, L"Arial", 1, Gdiplus::FontStyleBold, 25);
#endif
}

void VulkanRenderTextEx(const std::string& str, VulkanImage* image, const wchar_t* face, bool border, int style, float emsize)
{
	if (image == nullptr)
		return;

#ifdef _WIN32
	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(face);
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(border ? fill : outline);
	std::wstring				wstr(str.begin(), str.end());

	INT width = (INT)image->GetWidth();
	INT height = (INT)image->GetHeight();

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), (INT)wstr.length(), &family, style, emsize, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	if (border)
		graphics->DrawPath(&pen, &path);

	graphics->FillPath(&brush, &path);

	// copy to image
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));
	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

	void* imgdata = image->MapContents(0, 0);
	{
		memcpy(imgdata, data.Scan0, (size_t)width * height * 4);
	}
	image->UnmapContents();

	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
#endif
}

void VulkanTemporaryCommandBuffer(bool begin, bool wait, std::function<bool (VkCommandBuffer)> callback)
{
	if (callback == nullptr)
		return;

	VkCommandBuffer				cmdbuff		= NULL;
	VkCommandBufferAllocateInfo	cmdbuffinfo	= {};
	VkCommandBufferBeginInfo	begininfo	= {};
	VkSubmitInfo				submitinfo	= {};
	VkResult					res;

	cmdbuffinfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbuffinfo.pNext				= NULL;
	cmdbuffinfo.commandPool			= driverInfo.commandPool;
	cmdbuffinfo.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbuffinfo.commandBufferCount	= 1;

	res = vkAllocateCommandBuffers(driverInfo.device, &cmdbuffinfo, &cmdbuff);
	VK_ASSERT(res == VK_SUCCESS);

	if (begin) {
		begininfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begininfo.pNext				= NULL;
		begininfo.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begininfo.pInheritanceInfo	= NULL;

		res = vkBeginCommandBuffer(cmdbuff, &begininfo);
		VK_ASSERT(res == VK_SUCCESS);
	}

	bool success = callback(cmdbuff);

	if (!success) {
		// just delete it
		vkFreeCommandBuffers(driverInfo.device, driverInfo.commandPool, 1, &cmdbuff);
		return;
	}

	submitinfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitinfo.pNext				= NULL;
	submitinfo.waitSemaphoreCount	= 0;
	submitinfo.pWaitSemaphores		= NULL;
	submitinfo.pWaitDstStageMask	= 0;
	submitinfo.commandBufferCount	= 1;
	submitinfo.pCommandBuffers		= &cmdbuff;
	submitinfo.signalSemaphoreCount	= 0;
	submitinfo.pSignalSemaphores	= NULL;

	vkEndCommandBuffer(cmdbuff);

	res = vkQueueSubmit(driverInfo.graphicsQueue, 1, &submitinfo, VK_NULL_HANDLE);
	VK_ASSERT(res == VK_SUCCESS);

	if (wait) {
		res = vkQueueWaitIdle(driverInfo.graphicsQueue);
		VK_ASSERT(res == VK_SUCCESS);

		vkFreeCommandBuffers(driverInfo.device, driverInfo.commandPool, 1, &cmdbuff);
	} // else it's your job to delete it
}

VkVertexInputBindingDescription VulkanMakeBindingDescription(uint32_t binding, VkVertexInputRate inputrate, uint32_t stride)
{
	VkVertexInputBindingDescription bindingdesc;

	bindingdesc.binding		= binding;
	bindingdesc.inputRate	= inputrate;
	bindingdesc.stride		= stride;

	return std::move(bindingdesc);
}
