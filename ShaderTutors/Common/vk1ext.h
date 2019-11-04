
#ifndef _VK1EXT_H_
#define _VK1EXT_H_

#include <cassert>
#include <functional>
#include <set>
#include <map>
#include <vector>

#include <vulkan.h>
#include <Public/ShaderLang.h>

#include "orderedarray.hpp"
#include "3Dmath.h"

#ifdef _DEBUG
#	define VK_ASSERT(x)		assert(x)
#else
#	define VK_ASSERT(x)		{ if (!(x)) { throw 1; } }
#endif

#define VK_SAFE_RELEASE(x)	{ if ((x) != nullptr) { (x)->Release(); (x) = nullptr; } }

class VulkanImage;
class VulkanGraphicsPipeline;

// --- Enums ------------------------------------------------------------------

typedef enum VkMemoryPropertyFlagBitsEx {
	VK_MEMORY_PROPERTY_SHARED_BIT = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	VK_MEMORY_PROPERTY_SHARED_COHERENT_BIT = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
} VkMemoryPropertyFlagBitsEx;

enum VulkanMeshFlags {
	VK_MESH_32BIT = 1
};

// --- Structures -------------------------------------------------------------

struct VulkanDriverInfo
{
	VkInstance							instance;
	VkSurfaceKHR						surface;
	VkDevice							device;
	VkCommandPool						commandPool;
	VkSwapchainKHR						swapchain;
	VkQueue								graphicsQueue;		// this will be used for drawing
	VkDebugReportCallbackEXT			callback;

	VkPhysicalDevice*					devices;
	VkPhysicalDevice					selectedDevice;
	VkPhysicalDeviceProperties			deviceProps;
	VkPhysicalDeviceFeatures			deviceFeatures;
	VkPhysicalDeviceFeatures2			deviceFeatures2;	// additional features
	VkPhysicalDeviceMemoryProperties	memoryProps;
	VkPhysicalDeviceSubgroupProperties	subgroupProps;
	VkQueueFamilyProperties*			queueProps;			// for primary adapter
	VkSurfaceCapabilitiesKHR			surfaceCaps;		// for primary adapter
	VkPresentModeKHR*					presentModes;		// for primary adapter
	VkImage*							swapchainImages;
	VkImageView*						swapchainImageViews;

	VkFormat							format;				// backbuffer format
	uint32_t							numDevices;
	uint32_t							numPresentModes;
	uint32_t							numQueues;			// for primary adapter
	uint32_t							graphicsQueueID;
	uint32_t							computeQueueID;
	uint32_t							numSwapChainImages;

	// already declared in global scope...
	PFN_vkCreateDebugReportCallbackEXT	vkCreateDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT			vkDebugReportMessageEXT;
	PFN_vkDestroyDebugReportCallbackEXT	vkDestroyDebugReportCallbackEXT;
};

struct VulkanSubAllocation
{
	VkDeviceMemory	memory;
	VkDeviceSize	offset;
	uint32_t		heap;

	VulkanSubAllocation();

	inline operator bool() const {
		return (memory != NULL);
	}
};

struct VulkanAttributeRange
{
	uint32_t	primitiveType;	// must match pipeline IA topology
	uint32_t	attribId;
	uint32_t	indexStart;
	uint32_t	indexCount;
	uint32_t	vertexStart;
	uint32_t	vertexCount;
	bool		enabled;
};

struct VulkanMaterial
{
	Math::Color		diffuse;
	Math::Color		ambient;
	Math::Color		specular;
	Math::Color		emissive;
	float			power;
	VulkanImage*	texture;
	VulkanImage*	normalMap;

	VulkanMaterial();
	~VulkanMaterial();
};

/**
 * \brief Max memory allocation count is 4096 so we have to suballocate.
 */
class VulkanMemorySubAllocator
{
	struct AllocationRecord
	{
		VkDeviceSize	offset;
		VkDeviceSize	size;
		mutable bool	mapped;

		AllocationRecord() {
			offset = size = 0;
			mapped = false;
		}

		AllocationRecord(VkDeviceSize off) {
			offset = off;
			size = 0;
			mapped = false;
		}

		inline bool operator <(const AllocationRecord& other) const {
			return (offset < other.offset);
		}
	};

	typedef std::set<AllocationRecord> AllocationSet;

	struct MemoryBatch
	{
		AllocationSet		allocations;
		VkDeviceMemory		memory;
		VkDeviceSize		totalsize;
		mutable uint8_t*	mappedrange;
		uint32_t			heapindex;
		mutable uint16_t	mappedcount;
		bool				isoptimal;

		const AllocationRecord& FindRecord(const VulkanSubAllocation& alloc) const;

		inline bool operator <(const MemoryBatch& other) const {
			return (memory < other.memory);
		}
	};

	typedef OrderedArray<MemoryBatch> MemoryBatchArray;

private:
	static VulkanMemorySubAllocator* _inst;

	MemoryBatchArray batchesforheap[VK_MAX_MEMORY_HEAPS];

	VulkanMemorySubAllocator();
	~VulkanMemorySubAllocator();

	VkDeviceSize		AdjustBufferOffset(VkDeviceSize offset, VkDeviceSize alignment, VkBufferUsageFlags usageflags);
	VkDeviceSize		AdjustImageOffset(VkDeviceSize offset, VkDeviceSize alignment);
	const MemoryBatch&	FindBatchForAlloc(const VulkanSubAllocation& alloc);
	const MemoryBatch&	FindSuitableBatch(VkDeviceSize& outoffset, VkMemoryRequirements memreqs, VkFlags requirements, VkFlags usageflags, bool optimal);

public:
	static VulkanMemorySubAllocator& Instance();
	static void Release();
	
	VulkanSubAllocation	AllocateForBuffer(VkMemoryRequirements memreqs, VkFlags requirements, VkBufferUsageFlags usageflags);
	VulkanSubAllocation	AllocateForImage(VkMemoryRequirements memreqs, VkFlags requirements, VkImageTiling tiling);
	void				Deallocate(VulkanSubAllocation& alloc);
	uint32_t			GetMemoryTypeForFlags(uint32_t memtype, VkFlags requirements);
	void*				MapMemory(const VulkanSubAllocation& alloc, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
	void				UnmapMemory(const VulkanSubAllocation& alloc);
};

inline VulkanMemorySubAllocator& VulkanMemoryManager() {
	return VulkanMemorySubAllocator::Instance();
}

/**
 * \brief Buffer with arbitrary data.
 */
class VulkanBuffer
{
private:
	VkDescriptorBufferInfo	bufferinfo;
	VkMemoryRequirements	memreqs;
	VkMappedMemoryRange		mappedrange;

	VulkanSubAllocation		memory;
	VulkanSubAllocation		stagingmemory;

	VkBuffer				buffer;
	VkBuffer				stagingbuffer;

	VkDeviceSize			originalsize;
	VkFlags					exflags;
	void*					contents;

	VulkanBuffer();

public:
	~VulkanBuffer();

	static VulkanBuffer* Create(VkBufferUsageFlags usage, VkDeviceSize size, VkFlags flags);

	void* MapContents(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags = 0);
	void UnmapContents();
	void UploadToVRAM(VkCommandBuffer commandbuffer);
	void DeleteStagingBuffer();

	inline VkBuffer GetBuffer()									{ return buffer; }
	inline const VkDescriptorBufferInfo& GetBufferInfo() const	{ return bufferinfo; }
	inline VkDeviceSize GetSize() const							{ return memreqs.size; }
};

/**
 * \brief Instead of smart ptrs.
 */
class VulkanRefCountable
{
private:
	uint32_t refcount;

protected:
	VulkanRefCountable();
	virtual ~VulkanRefCountable();

public:
	void AddRef();
	void Release();
};

/**
 * \brief Texture or attachment.
 */
class VulkanImage : public VulkanRefCountable
{
	friend class VulkanPipelineBarrierBatch;

private:
	VkDescriptorImageInfo	imageinfo;	// NOTE: this is the expected GPU state
	VkMemoryRequirements	memreqs;
	VkExtent3D				extents;

	VkImage					image;
	VkImageView				imageview;
	VkSampler				sampler;
	VkFormat				format;
	VkAccessFlags*			accesses;	// NOTE: these are not the actual GPU state
	VkImageLayout*			layouts;	// NOTE: these are not the actual GPU state

	VulkanSubAllocation		memory;
	VulkanBuffer*			stagingbuffer;
	uint32_t				mipmapcount;
	uint32_t				slicecount;

	VulkanImage();
	~VulkanImage();

public:
	static VulkanImage* Create2D(VkFormat format, uint32_t width, uint32_t height, uint32_t miplevels, VkImageUsageFlags usage);
	static VulkanImage* CreateFromFile(const char* file, bool srgb);
	static VulkanImage* CreateFromDDS(const char* file, bool srgb);

	static size_t CalculateImageSizeAndMipmapCount(uint32_t& nummipsout, VkFormat format, uint32_t width, uint32_t height);
	static size_t CalculateSliceSize(VkFormat format, uint32_t width, uint32_t height);
	static size_t CalculateByteSize(VkFormat format);

	void* MapContents(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags = 0);
	void UnmapContents();
	void UploadToVRAM(VulkanPipelineBarrierBatch& barrier, VkCommandBuffer commandbuffer, bool generatemips = true);
	void DeleteStagingBuffer();

	VkAccessFlags GetAccess(uint32_t level, uint32_t slice) const;
	VkImageLayout GetLayout(uint32_t level, uint32_t slice) const;

	inline VkImage GetImage()									{ return image; }
	inline VkImageView GetImageView()							{ return imageview; }
	inline const VkDescriptorImageInfo* GetImageInfo() const	{ return &imageinfo; }
	
	inline VkSampler GetSampler()								{ return sampler; }
	inline VkFormat GetFormat() const							{ return format; }

	inline uint32_t GetMipMapCount() const						{ return mipmapcount; }
	inline uint32_t GetArraySize() const						{ return slicecount; }
	inline uint32_t GetWidth() const							{ return extents.width; }
	inline uint32_t GetHeight() const							{ return extents.height; }
};

/**
 * \brief Triangle list mesh.
 */
class VulkanMesh // : public VulkanRefCountable
{
private:
	Math::AABox							boundingbox;
	VulkanBuffer*						vertexbuffer;
	VulkanBuffer*						indexbuffer;
	VulkanBuffer*						uniformbuffer;
	VulkanAttributeRange*				subsettable;
	VulkanMaterial*						materials;

	VkDescriptorBufferInfo				unibufferinfo;

	uint8_t*							mappedvdata;
	uint8_t*							mappedidata;
	uint8_t*							mappedudata;
	VkDeviceSize						baseoffset;
	VkDeviceSize						indexoffset;
	VkDeviceSize						uniformoffset;
	VkDeviceSize						totalsize;
	uint32_t							vstride;
	uint32_t							vertexcount;
	uint32_t							indexcount;
	uint32_t							numsubsets;
	VkIndexType							indexformat;
	bool								inherited;

public:
	static VulkanMesh* LoadFromQM(const char* file, VulkanBuffer* buffer = nullptr, VkDeviceSize offset = 0);

	VulkanMesh(uint32_t numvertices, uint32_t numindices, uint32_t vertexstride, VulkanBuffer* buffer = nullptr, VkDeviceSize offset = 0, uint32_t flags = 0);
	~VulkanMesh();

	void Draw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline, bool rebind = true);
	void DrawSubset(VkCommandBuffer commandbuffer, uint32_t index, VulkanGraphicsPipeline* pipeline, bool rebind = true);
	void DrawSubsetInstanced(VkCommandBuffer commandbuffer, uint32_t index, VulkanGraphicsPipeline* pipeline, uint32_t numinstances, bool rebind = true);
	void EnableSubset(uint32_t index, bool enable);
	void UploadToVRAM(VulkanPipelineBarrierBatch& barrier, VkCommandBuffer commandbuffer);
	void DeleteStagingBuffers();
	void GenerateTangentFrame();

	void* GetVertexBufferPointer();
	void* GetIndexBufferPointer();
	void* GetUniformBufferPointer();

	inline VkDeviceSize GetTotalSize() const							{ return totalsize; }

	inline uint32_t GetVertexStride() const								{ return vstride; }
	inline uint32_t GetIndexStride() const								{ return (indexformat == VK_INDEX_TYPE_UINT16 ? 2 : 4); }
	inline uint32_t GetNumVertices() const								{ return vertexcount; }
	inline uint32_t GetNumPolygons() const								{ return indexcount / 3; }
	inline uint32_t GetNumSubsets() const								{ return numsubsets; }

	inline const VkDescriptorBufferInfo* GetUniformBufferInfo() const	{ return &unibufferinfo; }
	inline const VulkanMaterial& GetMaterial(uint32_t subset) const		{ return materials[subset]; }
	inline const Math::AABox& GetBoundingBox() const					{ return boundingbox; }
};

/**
 * \brief Don't load content items more than once.
 */
class VulkanContentRegistry
{
	typedef std::map<std::string, VulkanImage*> ImageMap;

private:
	static VulkanContentRegistry* _inst;

	ImageMap images;

	VulkanContentRegistry();
	~VulkanContentRegistry();

public:
	static VulkanContentRegistry& Instance();
	static void Release();

	void RegisterImage(const std::string& file, VulkanImage* image);
	void UnregisterImage(VulkanImage* image);

	VulkanImage* PointerImage(const std::string& file);
};

inline VulkanContentRegistry& VulkanContentManager() {
	return VulkanContentRegistry::Instance();
}

/**
 * \brief Render pass.
 */
class VulkanRenderPass
{
	typedef std::vector<VkAttachmentDescription> AttachmentArray;
	typedef std::vector<VkAttachmentReference> ReferenceArray;
	typedef std::vector<VkSubpassDependency> DependencyArray;
	typedef std::vector<uint32_t> PreserveArray;

private:
	AttachmentArray				attachmentdescs;
	DependencyArray				dependencies;
	ReferenceArray*				inputreferences;
	ReferenceArray*				colorreferences;
	PreserveArray*				preservereferences;
	VkAttachmentReference*		depthreferences;

	VkRenderPass				renderpass;
	VkClearValue*				clearcolors;
	uint32_t					framewidth;
	uint32_t					frameheight;
	uint32_t					numsubpasses;

public:
	VulkanRenderPass(uint32_t width, uint32_t height, uint32_t subpasses);
	~VulkanRenderPass();

	void AddAttachment(VkFormat format, VkAttachmentLoadOp loadop, VkAttachmentStoreOp storeop, VkImageLayout initiallayout, VkImageLayout finallayout);
	void AddDependency(uint32_t srcsubpass, VkAccessFlags srcaccess, VkPipelineStageFlags srcstage, uint32_t dstsubpass, VkAccessFlags dstaccess, VkPipelineStageFlags dststage, VkDependencyFlags flags);

	void AddSubpassInputReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout);
	void AddSubpassColorReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout);
	void AddSubpassPreserveReference(uint32_t subpass, uint32_t attachment);

	void SetSubpassDepthReference(uint32_t subpass, uint32_t attachment, VkImageLayout layout);
	
	void Begin(VkCommandBuffer commandbuffer, VkFramebuffer framebuffer, VkSubpassContents contents, const Math::Color& clearcolor, float cleardepth, uint8_t clearstencil);
	void End(VkCommandBuffer commandbuffer);
	void NextSubpass(VkCommandBuffer commandbuffer, VkSubpassContents contents);

	bool Assemble();

	inline VkRenderPass GetRenderPass()	{ return renderpass; }
};

/**
 * \brief For specialization constants.
 */
class VulkanSpecializationInfo
{
	typedef std::vector<VkSpecializationMapEntry> EntryArray;

private:
	VkSpecializationInfo	specinfo;
	EntryArray				entries;

public:
	VulkanSpecializationInfo();
	VulkanSpecializationInfo(const VulkanSpecializationInfo& other);
	~VulkanSpecializationInfo();

	void AddInt(uint32_t constantID, int32_t value);
	void AddUInt(uint32_t constantID, uint32_t value);
	void AddFloat(uint32_t constantID, float value);

	inline const VkSpecializationInfo* GetSpecializationInfo() const	{ return &specinfo; }
};

/**
 * \brief Common elements of pipelines.
 */
class VulkanBasePipeline
{
	typedef std::vector<VkPipelineShaderStageCreateInfo> ShaderStageArray;
	typedef std::vector<VkDescriptorSetLayoutBinding> DescSetLayoutBindingArray;
	typedef std::vector<VkDescriptorBufferInfo> DescBufferInfoArray;
	typedef std::vector<VkDescriptorImageInfo> DescImageInfoArray;
	typedef std::vector<VkPushConstantRange> PushConstantRangeArray;
	typedef std::vector<VkShaderModule> ShaderModuleArray;
	typedef std::vector<VkDescriptorSet> DescriptorSetArray;

	// NOTE: encapsulates layout, pool and sets (for easier handling)
	struct DescriptorSetGroup
	{
		VkDescriptorPool			descriptorpool;
		VkDescriptorSetLayout		descsetlayout;
		
		DescSetLayoutBindingArray	bindings;			// what kind of buffers will be bound
		DescriptorSetArray			descriptorsets;
		DescBufferInfoArray			descbufferinfos;	// bound buffers
		DescImageInfoArray			descimageinfos;		// bound images

		DescriptorSetGroup();

		void Reset();
	};

	typedef std::vector<DescriptorSetGroup> DescriptorSetGroupArray;
	typedef std::vector<VulkanSpecializationInfo> SpecInfoArray;

protected:
	typedef std::vector<uint32_t> SPIRVByteCode;

	struct BaseTemporaryData
	{
		ShaderStageArray		shaderstages;
		SpecInfoArray			specinfos;
		PushConstantRangeArray	pushconstantranges;

		BaseTemporaryData();
	};

	ShaderModuleArray			shadermodules;
	DescriptorSetGroupArray		descsetgroups;			// preallocated descriptor sets organized by layout
	DescriptorSetGroup			currentgroup;

	VkPipeline					pipeline;
	VkPipelineLayout			pipelinelayout;
	BaseTemporaryData*			basetempdata;

	VulkanBasePipeline();
	virtual ~VulkanBasePipeline();

	bool Assemble();

public:
	bool AddShader(VkShaderStageFlagBits type, const char* file, const VulkanSpecializationInfo& specinfo = VulkanSpecializationInfo());
	void AddPushConstantRange(VkShaderStageFlags stages, uint32_t offset, uint32_t size);

	// descriptor set related methods
	void AllocateDescriptorSets(uint32_t numsets);
	void SetDescriptorSetLayoutBufferBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage);
	void SetDescriptorSetLayoutImageBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage);
	void SetDescriptorSetGroupBufferInfo(uint32_t group, uint32_t binding, const VkDescriptorBufferInfo* info);
	void SetDescriptorSetGroupImageInfo(uint32_t group, uint32_t binding, const VkDescriptorImageInfo* info);
	void UpdateDescriptorSet(uint32_t group, uint32_t set);

	inline VkPipeline GetPipeline()											{ return pipeline; }
	inline VkPipelineLayout GetPipelineLayout()								{ return pipelinelayout; }
	inline const VkDescriptorSet* GetDescriptorSets(uint32_t group) const	{ return descsetgroups[group].descriptorsets.data(); }
	inline size_t GetNumDescriptorSets(uint32_t group) const				{ return descsetgroups[group].descriptorsets.size(); }
};

/**
 * \brief Graphics pipeline.
 */
class VulkanGraphicsPipeline : public VulkanBasePipeline
{
	typedef std::vector<VkPipelineColorBlendAttachmentState> ColorAttachmentBlendStateArray;
	typedef std::vector<VkVertexInputAttributeDescription> AttributeList;
	typedef std::vector<VkVertexInputBindingDescription> VertexInputBindingArray;

	struct GraphicsTemporaryData : BaseTemporaryData
	{
		AttributeList							attributes;			// vertex layout
		ColorAttachmentBlendStateArray			blendstates;		// blending mode of render targets
		VertexInputBindingArray					vertexbindings;		// bound vertex buffer descriptions

		VkPipelineDynamicStateCreateInfo		dynamicstate;
		VkPipelineVertexInputStateCreateInfo	vertexinputstate;
		VkPipelineInputAssemblyStateCreateInfo	inputassemblystate;
		VkPipelineRasterizationStateCreateInfo	rasterizationstate;
		VkPipelineColorBlendStateCreateInfo		colorblendstate;
		VkPipelineViewportStateCreateInfo		viewportstate;
		VkPipelineDepthStencilStateCreateInfo	depthstencilstate;
		VkPipelineMultisampleStateCreateInfo	multisamplestate;

		GraphicsTemporaryData();
	};

private:
	GraphicsTemporaryData*	tempdata;
	VkViewport				viewport;
	VkRect2D				scissor;

public:
	VulkanGraphicsPipeline();
	~VulkanGraphicsPipeline();

	bool Assemble(VkRenderPass renderpass, uint32_t subpass);

	// graphics related methods
	void SetInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
	void SetVertexInputBinding(uint32_t location, const VkVertexInputBindingDescription& desc);
	void SetInputAssemblerState(VkPrimitiveTopology topology, VkBool32 primitiverestart);
	void SetRasterizerState(VkPolygonMode fillmode, VkCullModeFlags cullmode);
	void SetDepthState(VkBool32 depthenable, VkBool32 depthwriteenable, VkCompareOp compareop);
	void SetBlendState(uint32_t attachment, VkBool32 enable, VkBlendOp colorop, VkBlendFactor srcblend, VkBlendFactor destblend);
	void SetViewport(float x, float y, float width, float height, float minz = 0.0f, float maxz = 1.0f);
	void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

	inline const VkViewport* GetViewport() const	{ return &viewport; }
	inline const VkRect2D* GetScissor() const		{ return &scissor; }
};

/**
 * \brief Compute pipeline.
 */
class VulkanComputePipeline : public VulkanBasePipeline
{
public:
	VulkanComputePipeline();
	~VulkanComputePipeline();

	bool Assemble();
};

/**
 * \brief Compact class for barriers.
 */
class VulkanPipelineBarrierBatch
{
private:
	VkPipelineStageFlags	src;
	VkPipelineStageFlags	dst;

	std::vector<VkBufferMemoryBarrier>	buffbarriers;
	std::vector<VkImageMemoryBarrier>	imgbarriers;

public:
	VulkanPipelineBarrierBatch(VkPipelineStageFlags srcstage, VkPipelineStageFlags dststage);

	void BufferAccessBarrier(VkBuffer buffer, VkAccessFlags srcaccess, VkAccessFlags dstaccess, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	void ImageLayoutTransition(VkImage image, VkAccessFlags srcaccess, VkAccessFlags dstaccess, VkImageAspectFlags aspectmask, VkImageLayout oldlayout, VkImageLayout newlayout, uint32_t firstmip, uint32_t mipcount, uint32_t firstslice, uint32_t slicecount);
	
	void ImageLayoutTransition(VulkanImage* image, VkAccessFlags dstaccess, VkImageAspectFlags aspectmask, VkImageLayout newlayout, uint32_t firstmip, uint32_t mipcount, uint32_t firstslice, uint32_t slicecount);
	void Enlist(VkCommandBuffer commandbuffer);
	void Reset();
	void Reset(VkPipelineStageFlags srcstage, VkPipelineStageFlags dststage);
};

/**
 * \brief Makes 2D rendering easier.
 */
class VulkanScreenQuad
{
private:
	Math::Matrix transform;
	VulkanGraphicsPipeline*	pipeline;

public:
	VulkanScreenQuad(VkRenderPass renderpass, uint32_t subpass, VulkanImage* image, const char* psfile = "../../Media/ShadersVK/screenquad.frag");
	~VulkanScreenQuad();

	void Draw(VkCommandBuffer commandbuffer);
	void SetTextureMatrix(const Math::Matrix& m);

	inline VulkanGraphicsPipeline* GetPipeline()	{ return pipeline; }
};

// NOTE: filled by application
extern VulkanDriverInfo driverInfo;

/**
 * \brief Makes presentation easier.
 */
class VulkanPresentationEngine
{
public:
	static constexpr int NUM_QUEUED_FRAMES = 2;

private:
	VkCommandBuffer		commandbuffers[NUM_QUEUED_FRAMES];
	VkSemaphore			acquiresemas[NUM_QUEUED_FRAMES];	// for vkAcquireNextImageKHR
	VkSemaphore			presentsemas[NUM_QUEUED_FRAMES];	// for vkQueuePresentKHR
	VkFence				fences[NUM_QUEUED_FRAMES];			// to know when a submit finished
	VkRenderPass		renderpass;
	VkFramebuffer*		framebuffers;

	VulkanImage*		depthbuffer;
	uint32_t			buffersinflight;
	uint32_t			currentframe;
	uint32_t			currentdrawable;

public:
	std::function<void (uint32_t)> FrameFinished;

	static bool QueryFormatSupport(VkFormat format, VkFormatFeatureFlags features);

	VulkanPresentationEngine(uint32_t width, uint32_t height);
	~VulkanPresentationEngine();

	uint32_t GetNextDrawable();
	void Present();

	inline uint32_t GetCurrentFrame() const					{ return currentframe; }
	inline VkCommandBuffer GetCommandBuffer(uint32_t index)	{ return commandbuffers[index]; }
	inline VkRenderPass GetRenderPass()						{ return renderpass; }
	inline VkFramebuffer GetFramebuffer(uint32_t index)		{ return framebuffers[index]; }
	inline VkImage GetSwapchainImage(uint32_t index)		{ return driverInfo.swapchainImages[index]; }
	inline VulkanImage* GetDepthBuffer()					{ return depthbuffer; }
};

// --- Functions --------------------------------------------------------------

void VulkanRenderText(const std::string& str, VulkanImage* image);
void VulkanRenderTextEx(const std::string& str, VulkanImage* image, const wchar_t* face, bool border, int style, float emsize);
void VulkanTemporaryCommandBuffer(bool begin, bool wait, std::function<bool (VkCommandBuffer)> callback);

VkVertexInputBindingDescription VulkanMakeBindingDescription(uint32_t binding, VkVertexInputRate inputrate, uint32_t stride);

#endif
