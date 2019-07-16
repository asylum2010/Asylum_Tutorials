
#pragma comment(lib, "vulkan-1.lib")

#ifdef _DEBUG
#	pragma comment(lib, "glslangd.lib")
#	pragma comment(lib, "OGLCompilerd.lib")
#	pragma comment(lib, "OSDependentd.lib")
#	pragma comment(lib, "SPIRVd.lib")
#	pragma comment(lib, "SPIRV-Toolsd.lib")
#	pragma comment(lib, "SPIRV-Tools-optd.lib")
#	pragma comment(lib, "HLSLd.lib")
#else
#	pragma comment(lib, "glslang.lib")
#	pragma comment(lib, "OGLCompiler.lib")
#	pragma comment(lib, "OSDependent.lib")
#	pragma comment(lib, "SPIRV.lib")
#	pragma comment(lib, "SPIRV-Tools.lib")
#	pragma comment(lib, "SPIRV-Tools-opt.lib")
#	pragma comment(lib, "HLSL.lib")
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\vk1ext.h"
#include "..\Common\basiccamera.h"

#define OBJECT_GRID_SIZE	64		// nxn objects
#define TILE_GRID_SIZE		32		// kxk tiles
#define SPACING				0.4f
#define CAMERA_SPEED		0.05f

// helper macros
#define TITLE				"Shader sample 71: Draw call batching"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct CombinedUniformData
{
	// byte offset 0
	struct DrawBatchUniformData {
		Math::Matrix viewproj;
		Math::Vector4 lightpos;
		Math::Vector4 eyepos;
	} drawbatchuniforms;	// 96 B

	Math::Vector4 padding1[10];	// 160 B

	// byte offset 256
	struct DebugUniformData {
		Math::Matrix viewproj;
		Math::Color color;
	} debuguniforms;	// 80 B

	Math::Vector4 padding2[11];	// 176 B
};

struct MaterialData
{
	Math::Color color;
	Math::Vector4 padding[15];
};

// sample classes
class SceneObjectPrototype
{
private:
	VulkanMesh* mesh;

public:
	SceneObjectPrototype(const char* filename, VulkanBuffer* storage = 0, VkDeviceSize storageoffset = 0);
	~SceneObjectPrototype();

	void Draw(VkCommandBuffer commandbuffer);

	inline VulkanMesh* GetMesh()				{ return mesh; }
	inline const VulkanMesh* GetMesh() const	{ return mesh; }
};

class SceneObject
{
private:
	Math::Matrix	world;
	uint32_t		materialoffset;
	bool			encoded;

	SceneObjectPrototype* proto;

public:
	SceneObject(SceneObjectPrototype* prototype);

	void Draw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline);
	void GetBoundingBox(Math::AABox& outbox) const;

	inline bool IsEncoded() const						{ return encoded; }
	inline Math::Matrix& GetTransform()					{ return world; }
	inline const SceneObjectPrototype* GetProto() const	{ return proto; }

	inline void SetMaterialOffset(uint32_t offset)		{ materialoffset = offset; }
	inline void SetEncoded(bool value)					{ encoded = value; }
};

class DrawBatch
{
public:
	struct TileObject {
		SceneObject*	object;
		bool			encoded[2];
	};

	typedef std::vector<TileObject> TileObjectArray;

private:
	TileObjectArray	tileobjects;
	Math::AABox		boundingbox;
	VkCommandBuffer	commandbuffers[2];
	bool			markedfordiscard[2];
	bool			discarded[2];
	uint32_t		lastenqueuedframe;

public:
	DrawBatch();
	~DrawBatch();

	void AddObject(SceneObject* obj);
	void DebugDraw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline);
	void Discard(uint32_t finishedframe);
	void Regenerate(uint32_t currentimage, VkRenderPass renderpass, VkFramebuffer framebuffer, VulkanGraphicsPipeline* pipeline);

	VkCommandBuffer SetForRender(uint32_t frameid, uint32_t currentimage);

	inline void MarkForDiscard(uint32_t currentimage)		{ markedfordiscard[currentimage] = true; }

	inline const Math::AABox& GetBoundingBox() const		{ return boundingbox; }
	inline const TileObjectArray& GetObjects() const		{ return tileobjects; }
	inline bool IsDiscarded(uint32_t currentimage) const	{ return discarded[currentimage]; }
	inline bool IsMarkedForDiscard() const					{ return (markedfordiscard[0] || markedfordiscard[1]); }
};

typedef std::vector<SceneObject*> ObjectArray;
typedef std::vector<DrawBatch*> BatchArray;

// sample variables
Application*				app					= nullptr;
VkDevice					device				= nullptr;
VkFramebuffer*				mainframebuffers	= nullptr;

VulkanBuffer*				objectstorage		= nullptr;	// store all meshes in one buffer
VulkanBuffer*				uniformbuffer		= nullptr;
VulkanBuffer*				materialbuffer		= nullptr;
VulkanMesh*					debugmesh			= nullptr;
VulkanGraphicsPipeline*		drawbatching		= nullptr;
VulkanGraphicsPipeline*		simplecolor			= nullptr;
VulkanRenderPass*			mainrenderpass		= nullptr;
VulkanPresentationEngine*	presenter			= nullptr;
VulkanScreenQuad*			screenquad			= nullptr;
VulkanImage*				helptext			= nullptr;

SceneObjectPrototype*		prototypes[3]		= { nullptr };
ObjectArray					sceneobjects;
BatchArray					tiles;
BatchArray					visibletiles;
BasicCamera					debugcamera;
uint32_t					totalpolys			= 0;
float						totalwidth;
float						totaldepth;
float						totalheight;
bool						debugmode			= false;

void FrameFinished(uint32_t frameid);
void UpdateTiles(const Math::Matrix& viewproj, uint32_t currentimage);

// --- SceneObjectPrototype impl ----------------------------------------------

SceneObjectPrototype::SceneObjectPrototype(const char* filename, VulkanBuffer* storage, VkDeviceSize storageoffset)
{
	mesh = VulkanMesh::LoadFromQM(filename, storage, storageoffset);
	
	VK_ASSERT(mesh);
	VK_ASSERT(mesh->GetVertexStride() == 32);
}

SceneObjectPrototype::~SceneObjectPrototype()
{
	delete mesh;
}

void SceneObjectPrototype::Draw(VkCommandBuffer commandbuffer)
{
	mesh->Draw(commandbuffer, 0);
}

// --- SceneObject impl -------------------------------------------------------

SceneObject::SceneObject(SceneObjectPrototype* prototype)
{
	VK_ASSERT(prototype);

	proto = prototype;
	materialoffset = 0;
	encoded = false;

	Math::MatrixIdentity(world);
}

void SceneObject::Draw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline)
{
	if (!encoded) {
		vkCmdPushConstants(commandbuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 64, world);
		vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, pipeline->GetDescriptorSets(0), 1, &materialoffset);

		proto->Draw(commandbuffer);
		encoded = true;
	}
}

void SceneObject::GetBoundingBox(Math::AABox& outbox) const
{
	outbox = proto->GetMesh()->GetBoundingBox();
	outbox.TransformAxisAligned(world);
}

// --- DrawBatch impl ---------------------------------------------------------

DrawBatch::DrawBatch()
{
	VkCommandBufferAllocateInfo cmdbuffinfo = {};
	VkResult res;

	cmdbuffinfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbuffinfo.pNext				= NULL;
	cmdbuffinfo.commandPool			= driverInfo.commandPool;
	cmdbuffinfo.level				= VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	cmdbuffinfo.commandBufferCount	= 2;

	res = vkAllocateCommandBuffers(driverInfo.device, &cmdbuffinfo, commandbuffers);
	VK_ASSERT(res == VK_SUCCESS);

	markedfordiscard[0] = false;
	markedfordiscard[1] = false;

	discarded[0] = true;
	discarded[1] = true;
}

DrawBatch::~DrawBatch()
{
	vkFreeCommandBuffers(driverInfo.device, driverInfo.commandPool, 2, commandbuffers);
	tileobjects.clear();
}

void DrawBatch::AddObject(SceneObject* obj)
{
	Math::AABox	objbox;
	TileObject	tileobj;

	if (tileobjects.size() >= tileobjects.capacity())
		tileobjects.reserve(tileobjects.capacity() + 16);

	tileobj.object = obj;
	tileobj.encoded[0] = false;
	tileobj.encoded[1] = false;

	tileobjects.push_back(tileobj);
	obj->GetBoundingBox(objbox);

	boundingbox.Add(objbox.Min);
	boundingbox.Add(objbox.Max);
}

void DrawBatch::DebugDraw(VkCommandBuffer commandbuffer, VulkanGraphicsPipeline* pipeline)
{
	for (size_t j = 0; j < tileobjects.size(); ++j)
		tileobjects[j].object->Draw(commandbuffer, pipeline);
}

void DrawBatch::Discard(uint32_t finishedframe)
{
	if (finishedframe == lastenqueuedframe) {
		for (int j = 0; j < 2; ++j) {
			if (markedfordiscard[j] && !discarded[j]) {
				for (size_t i = 0; i < tileobjects.size(); ++i)
					tileobjects[i].encoded[j] = false;

				vkResetCommandBuffer(commandbuffers[j], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
				
				discarded[j] = true;
				markedfordiscard[j] = false;
			}
		}
	}
}

void DrawBatch::Regenerate(uint32_t currentimage, VkRenderPass renderpass, VkFramebuffer framebuffer, VulkanGraphicsPipeline* pipeline)
{
	if (!discarded[currentimage]) {
		// not deleted yet, mark them encoded
		for (size_t i = 0; i < tileobjects.size(); ++i) {
			if( tileobjects[i].encoded[currentimage] )
				tileobjects[i].object->SetEncoded(true);
		}

		markedfordiscard[currentimage] = false;
		return;
	}

	VkCommandBufferInheritanceInfo	inheritanceinfo	= {};
	VkCommandBufferBeginInfo		begininfo		= {};
	VkResult res;

	inheritanceinfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceinfo.pNext					= NULL;
	inheritanceinfo.renderPass				= renderpass;
	inheritanceinfo.subpass					= 0;
	inheritanceinfo.occlusionQueryEnable	= VK_FALSE;
	inheritanceinfo.queryFlags				= 0;
	inheritanceinfo.pipelineStatistics		= 0;
	inheritanceinfo.framebuffer				= framebuffer;

	begininfo.sType							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.pNext							= NULL;
	begininfo.flags							= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT|VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	begininfo.pInheritanceInfo				= &inheritanceinfo;

	res = vkBeginCommandBuffer(commandbuffers[currentimage], &begininfo);
	VK_ASSERT(res == VK_SUCCESS);
	{
		vkCmdBindPipeline(commandbuffers[currentimage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
		vkCmdSetViewport(commandbuffers[currentimage], 0, 1, pipeline->GetViewport());
		vkCmdSetScissor(commandbuffers[currentimage], 0, 1, pipeline->GetScissor());

		for (size_t j = 0; j < tileobjects.size(); ++j) {
			bool alreadyencoded = tileobjects[j].object->IsEncoded();
			tileobjects[j].encoded[currentimage] = !alreadyencoded;

			if (!alreadyencoded) {
				// don't encode more than once
				tileobjects[j].object->Draw(commandbuffers[currentimage], pipeline);
			}
		}
	}
	vkEndCommandBuffer(commandbuffers[currentimage]);

	discarded[currentimage] = false;
}

VkCommandBuffer DrawBatch::SetForRender(uint32_t frameid, uint32_t currentimage)
{
	lastenqueuedframe = frameid;
	return commandbuffers[currentimage];
}

// --- Sample impl ------------------------------------------------------------

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	presenter->FrameFinished = FrameFinished;

	// material data
	materialbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 4 * sizeof(MaterialData), VK_MEMORY_PROPERTY_SHARED_BIT);
	uniformbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(CombinedUniformData), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	MaterialData* matdata = (MaterialData*)materialbuffer->MapContents(0, 0);
	{
		matdata[0].color = Math::Color(1, 0, 0, 1);
		matdata[1].color = Math::Color(0, 1, 0, 1);
		matdata[2].color = Math::Color(0, 0, 1, 1);
		matdata[3].color = Math::Color(1, 0.5f, 0, 1);
	}
	materialbuffer->UnmapContents();

	// object prototypes
	Math::Matrix objectscales[3];
	Math::Matrix translation, rotation;
	Math::AABox objectboxes[3];
	VkDeviceSize offset = 0;

	Math::MatrixScaling(objectscales[0], 1, 1, 1);
	Math::MatrixScaling(objectscales[1], 0.5f, 0.5f, 0.5f);
	Math::MatrixScaling(objectscales[2], 0.55f, 0.55f, 0.55f);

	prototypes[0] = new SceneObjectPrototype("../../Media/MeshesQM/teapot.qm", objectstorage, offset);				// 375552	[3.216800, 1.575000, 2.000000]
	offset += prototypes[0]->GetMesh()->GetTotalSize();

	prototypes[1] = new SceneObjectPrototype("../../Media/MeshesQM/reventon/reventon.qm", objectstorage, offset);	// 1292288	[4.727800, 1.143600, 2.175000]
	offset += prototypes[1]->GetMesh()->GetTotalSize();

	prototypes[2] = new SceneObjectPrototype("../../Media/MeshesQM/zonda.qm", objectstorage, offset);				// 752128	[2.055100, 1.061800, 4.229200]
	offset += prototypes[2]->GetMesh()->GetTotalSize();

	objectboxes[0] = prototypes[0]->GetMesh()->GetBoundingBox();
	objectboxes[1] = prototypes[1]->GetMesh()->GetBoundingBox();
	objectboxes[2] = prototypes[2]->GetMesh()->GetBoundingBox();

	objectboxes[0].TransformAxisAligned(objectscales[0]);
	objectboxes[1].TransformAxisAligned(objectscales[1]);
	objectboxes[2].TransformAxisAligned(objectscales[2]);

	srand(0);

	totalwidth = OBJECT_GRID_SIZE * 3.2168f + (OBJECT_GRID_SIZE - 1) * SPACING;
	totaldepth = OBJECT_GRID_SIZE * 2.0f + (OBJECT_GRID_SIZE - 1) * SPACING;
	totalheight = 5;

	for (size_t i = 0; i < OBJECT_GRID_SIZE; ++i) {
		for (size_t j = 0; j < OBJECT_GRID_SIZE; ++j) {
			int index = rand() % 3;
			float angle = Math::DegreesToRadians((float)(rand() % 360));

			SceneObjectPrototype* proto = prototypes[index];

			sceneobjects.push_back(new SceneObject(proto));
			totalpolys += proto->GetMesh()->GetNumPolygons();

			Math::MatrixRotationAxis(rotation, angle, 0, 1, 0);
			Math::MatrixTranslation(translation, i * (3.2168f + SPACING) - totalwidth * 0.5f, 0, j * (2.0f + SPACING) - totaldepth * 0.5f);

			translation._42 = -objectboxes[index].Min.y;	// so that they start at 0

			Math::MatrixMultiply(sceneobjects.back()->GetTransform(), objectscales[index], rotation);
			Math::MatrixMultiply(sceneobjects.back()->GetTransform(), sceneobjects.back()->GetTransform(), translation);

			sceneobjects.back()->SetMaterialOffset((rand() % 4) * sizeof(MaterialData));
		}
	}

	printf("Total number of polygons: %u\n", totalpolys);

	// create debug mesh
	debugmesh = new VulkanMesh(8, 24, 12);

	Math::Vector3* vdata = (Math::Vector3*)debugmesh->GetVertexBufferPointer();

	vdata[0] = Math::Vector3(-1, -1, -1);
	vdata[1] = Math::Vector3(-1, -1, 1);
	vdata[2] = Math::Vector3(-1, 1, -1);
	vdata[3] = Math::Vector3(-1, 1, 1);

	vdata[4] = Math::Vector3(1, -1, -1);
	vdata[5] = Math::Vector3(1, -1, 1);
	vdata[6] = Math::Vector3(1, 1, -1);
	vdata[7] = Math::Vector3(1, 1, 1);

	uint16_t* idata = (uint16_t*)debugmesh->GetIndexBufferPointer();

	uint16_t wireindices[] = {
		0, 1, 1, 3, 3, 2,
		2, 0, 0, 4, 4, 6,
		6, 2, 4, 5, 5, 7,
		7, 6, 7, 3, 1, 5
	};

	memcpy(idata, wireindices, 24 * sizeof(uint16_t));

	// create main render pass
	mainrenderpass = new VulkanRenderPass(screenwidth, screenheight, 2);

	mainrenderpass->AddAttachment(driverInfo.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->AddAttachment(presenter->GetDepthBuffer()->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	mainrenderpass->AddSubpassColorReference(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->SetSubpassDepthReference(0, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	mainrenderpass->AddSubpassColorReference(1, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->SetSubpassDepthReference(1, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	mainrenderpass->AddDependency(
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		1,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0);

	VK_ASSERT(mainrenderpass->Assemble());

	// create draw batching pipeline
	drawbatching = new VulkanGraphicsPipeline();

	VK_ASSERT(drawbatching->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/drawbatching.vert"));
	VK_ASSERT(drawbatching->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/drawbatching.frag"));

	drawbatching->SetInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);	// position
	drawbatching->SetInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12);	// normal
	drawbatching->SetInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, 24);		// texcoord

	drawbatching->SetVertexInputBinding(0, VulkanMakeBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, 32));
	drawbatching->AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, 64);

	VkDescriptorBufferInfo unibuffinfo;
	VkDescriptorBufferInfo matbuffinfo;

	unibuffinfo.buffer = uniformbuffer->GetBuffer();
	unibuffinfo.offset = offsetof(CombinedUniformData, drawbatchuniforms);
	unibuffinfo.range = sizeof(CombinedUniformData::DrawBatchUniformData);

	matbuffinfo.buffer = materialbuffer->GetBuffer();
	matbuffinfo.offset = 0;
	matbuffinfo.range = sizeof(MaterialData);

	drawbatching->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	drawbatching->SetDescriptorSetLayoutBufferBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
	
	drawbatching->AllocateDescriptorSets(1);

	drawbatching->SetDescriptorSetGroupBufferInfo(0, 0, &unibuffinfo);
	drawbatching->SetDescriptorSetGroupBufferInfo(0, 1, &matbuffinfo);

	drawbatching->UpdateDescriptorSet(0, 0);

	drawbatching->SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	drawbatching->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	drawbatching->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(drawbatching->Assemble(mainrenderpass->GetRenderPass(), 0));

	// create debug pipeline
	simplecolor = new VulkanGraphicsPipeline();

	VK_ASSERT(simplecolor->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/simplecolor.vert"));
	VK_ASSERT(simplecolor->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/simplecolor.frag"));

	simplecolor->SetInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);	// position
	simplecolor->SetVertexInputBinding(0, VulkanMakeBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, debugmesh->GetVertexStride()));

	unibuffinfo.buffer = uniformbuffer->GetBuffer();
	unibuffinfo.offset = offsetof(CombinedUniformData, debuguniforms);
	unibuffinfo.range = sizeof(CombinedUniformData::DebugUniformData);

	simplecolor->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	simplecolor->AllocateDescriptorSets(1);

	simplecolor->SetDescriptorSetGroupBufferInfo(0, 0, &unibuffinfo);
	simplecolor->UpdateDescriptorSet(0, 0);

	simplecolor->AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, 80);

	simplecolor->SetInputAssemblerState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE);
	simplecolor->SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	simplecolor->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	simplecolor->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(simplecolor->Assemble(mainrenderpass->GetRenderPass(), 1));

	// create tiles
	Math::AABox	tilebox;
	Math::AABox	objbox;
	float		tilewidth = totalwidth / TILE_GRID_SIZE;
	float		tiledepth = totaldepth / TILE_GRID_SIZE;

	tiles.resize(TILE_GRID_SIZE * TILE_GRID_SIZE, 0);

	printf("Generatig tiles...\n");

	for (size_t i = 0; i < TILE_GRID_SIZE; ++i) {
		for (size_t j = 0; j < TILE_GRID_SIZE; ++j) {
			tilebox.Min = Math::Vector3(totalwidth * -0.5f + j * tilewidth, 0, totaldepth * -0.5f + i * tiledepth);
			tilebox.Max = Math::Vector3(totalwidth * -0.5f + (j + 1) * tilewidth, totalheight, totaldepth * -0.5f + (i + 1) * tiledepth);

			DrawBatch* batch = (tiles[i * TILE_GRID_SIZE + j] = new DrawBatch());

			// this is REALLY slow...
			for (size_t k = 0; k < sceneobjects.size(); ++k) {
				sceneobjects[k]->GetBoundingBox(objbox);

				if (tilebox.Intersects(objbox))
					batch->AddObject(sceneobjects[k]);
			}
		}
	}

	visibletiles.reserve(tiles.size());

	// help text
	helptext = VulkanImage::Create2D(VK_FORMAT_B8G8R8A8_UNORM, 512, 512, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);

	VulkanRenderText("D - Toggle debug camera", helptext);

	// upload to device
	VulkanTemporaryCommandBuffer(true, true, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		{
			// NOTE: also transitions textures
			prototypes[0]->GetMesh()->UploadToVRAM(barrier, transfercmd);
			prototypes[1]->GetMesh()->UploadToVRAM(barrier, transfercmd);
			prototypes[2]->GetMesh()->UploadToVRAM(barrier, transfercmd);

			debugmesh->UploadToVRAM(barrier, transfercmd);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			materialbuffer->UploadToVRAM(transfercmd);
			helptext->UploadToVRAM(barrier, transfercmd, false);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			barrier.ImageLayoutTransition(helptext, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
		}
		barrier.Enlist(transfercmd);
		
		return true;
	});

	prototypes[0]->GetMesh()->DeleteStagingBuffers();
	prototypes[1]->GetMesh()->DeleteStagingBuffers();
	prototypes[2]->GetMesh()->DeleteStagingBuffers();

	debugmesh->DeleteStagingBuffers();
	materialbuffer->DeleteStagingBuffer();
	helptext->DeleteStagingBuffer();

	screenquad = new VulkanScreenQuad(mainrenderpass->GetRenderPass(), 1, helptext);

	// create main render pass framebuffers
	VkImageView				fbattachments[2];
	VkFramebufferCreateInfo	framebufferinfo = {};
	VkResult				res;

	mainframebuffers = new VkFramebuffer[driverInfo.numSwapChainImages];

	framebufferinfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferinfo.pNext			= NULL;
	framebufferinfo.renderPass		= mainrenderpass->GetRenderPass();
	framebufferinfo.attachmentCount	= ARRAY_SIZE(fbattachments);
	framebufferinfo.pAttachments	= fbattachments;
	framebufferinfo.width			= screenwidth;
	framebufferinfo.height			= screenheight;
	framebufferinfo.layers			= 1;

	fbattachments[1] = presenter->GetDepthBuffer()->GetImageView();

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		fbattachments[0] = driverInfo.swapchainImageViews[i];

		res = vkCreateFramebuffer(device, &framebufferinfo, NULL, &mainframebuffers[i]);
		VK_ASSERT(res == VK_SUCCESS);
	}

	// setup debug camera
	Math::AABox worldbox;
	Math::Vector3 center;

	worldbox.Min = Math::Vector3(totalwidth * -0.5f, totalheight * -0.5f, totaldepth * -0.5f);
	worldbox.Max = Math::Vector3(totalwidth * 0.5f, totalheight * 0.5f, totaldepth * 0.5f);

	worldbox.GetCenter(center);

	debugcamera.SetAspect((float)screenwidth / screenheight);
	debugcamera.SetFov(Math::PI / 3);
	debugcamera.SetPosition(center[0], center[1], center[2]);
	debugcamera.SetDistance(Math::Vec3Distance(worldbox.Min, worldbox.Max) * 0.65f);
	debugcamera.SetClipPlanes(0.1f, Math::Vec3Distance(worldbox.Min, worldbox.Max) * 2);
	debugcamera.OrbitRight(0);
	debugcamera.OrbitUp(Math::DegreesToRadians(30));
	debugcamera.SetZoomLimits(1.0f, Math::Vec3Distance(worldbox.Min, worldbox.Max));

	return true;
}

void UninitScene()
{
	vkQueueWaitIdle(driverInfo.graphicsQueue);

	for (size_t i = 0; i < tiles.size(); ++i)
		delete tiles[i];

	for (size_t i = 0; i < sceneobjects.size(); ++i)
		delete sceneobjects[i];

	for (size_t i = 0; i < ARRAY_SIZE(prototypes); ++i)
		delete prototypes[i];

	tiles.clear();
	tiles.swap(BatchArray());

	sceneobjects.clear();
	sceneobjects.swap(ObjectArray());

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		if (mainframebuffers[i])
			vkDestroyFramebuffer(device, mainframebuffers[i], NULL);
	}

	delete[] mainframebuffers;

	delete mainrenderpass;
	delete objectstorage;
	delete uniformbuffer;
	delete materialbuffer;
	delete debugmesh;
	delete drawbatching;
	delete simplecolor;
	delete screenquad;

	VK_SAFE_RELEASE(helptext);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCodeD:
		debugmode = !debugmode;
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (debugmode) {
		if (state & MouseButtonLeft) {
			debugcamera.OrbitRight(Math::DegreesToRadians(dx) * 0.5f);
			debugcamera.OrbitUp(Math::DegreesToRadians(dy) * 0.5f);
		} else if (state & MouseButtonMiddle) {
			float scale = debugcamera.GetDistance() / 10.0f;
			float amount = 1e-3f + scale * (0.1f - 1e-3f);

			debugcamera.PanRight(dx * -amount);
			debugcamera.PanUp(dy * amount);
		}
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	float zoomspeed = debugcamera.GetFarPlane() / 20.0f;
	debugcamera.Zoom(dz * zoomspeed);
}

void UpdateTiles(const Math::Matrix& viewproj, uint32_t currentimage)
{
	// TODO: make this multithreaded
	Math::Vector4 planes[6];

	Math::FrustumPlanes(planes, viewproj);
	visibletiles.clear();

	// mark all objects non-encoded
	for (size_t i = 0; i < sceneobjects.size(); ++i)
		sceneobjects[i]->SetEncoded(false);

	for (size_t i = 0; i < tiles.size(); ++i) {
		const Math::AABox& tilebox = tiles[i]->GetBoundingBox();

		if (Math::FrustumIntersect(planes, tilebox) > 0) {
			visibletiles.push_back(tiles[i]);

			// mark tile-encoded objects encoded (to avoid double encoding)
			if (!tiles[i]->IsDiscarded(currentimage))
				tiles[i]->Regenerate(currentimage, mainrenderpass->GetRenderPass(), mainframebuffers[currentimage], drawbatching);
		} else {
			tiles[i]->MarkForDiscard(currentimage);
		}
	}

	// regenerate new tiles
	for (size_t i = 0; i < visibletiles.size(); ++i) { 
		if (visibletiles[i]->IsDiscarded(currentimage))
			visibletiles[i]->Regenerate(currentimage, mainrenderpass->GetRenderPass(), mainframebuffers[currentimage], drawbatching);
	}
}

void Update(float delta)
{
	debugcamera.Update(delta);
}

void Test(Math::Matrix& out, const Math::Vector3& eye, const Math::Vector3& look, const Math::Vector3& up)
{
	Math::Vector3 x, y, z;

	z[0] = eye[0] - look[0];
	z[1] = eye[1] - look[1];
	z[2] = eye[2] - look[2];

	Math::Vec3Normalize(z, z);
	Math::Vec3Cross(x, up, z);

	Math::Vec3Normalize(x, x);
	Math::Vec3Cross(y, z, x);

	out._11 = x[0];		out._12 = y[0];		out._13 = z[0];		out._14 = 0.0f;
	out._21 = x[1];		out._22 = y[1];		out._23 = z[1];		out._24 = 0.0f;
	out._31 = x[2];		out._32 = y[2];		out._33 = z[2];		out._34 = 0.0f;

	out._41 = -Math::Vec3Dot(x, eye);
	out._42 = -Math::Vec3Dot(y, eye);
	out._43 = -Math::Vec3Dot(z, eye);
	out._44 = 1.0f;
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	CombinedUniformData			uniforms;
	Math::Matrix				debugworld;
	Math::Matrix				view, proj;
	Math::Vector3				eye, look, up, tangent;

	uint32_t					currentdrawable	= presenter->GetNextDrawable();
	VkCommandBufferBeginInfo	begininfo		= {};
	VkRenderPassBeginInfo		passbegininfo	= {};
	VkCommandBuffer				primarycmdbuff	= presenter->GetCommandBuffer(currentdrawable);
	VkImage						swapchainimg	= presenter->GetSwapchainImage(currentdrawable);
	VkImage						depthbuffer		= presenter->GetDepthBuffer()->GetImage();
	VkResult					res;

	// setup transforms
	uint32_t	screenwidth		= app->GetClientWidth();
	uint32_t	screenheight	= app->GetClientHeight();
	float		halfw			= totalwidth * 0.45f;
	float		halfd			= totaldepth * 0.45f;
	float		t				= time * (CAMERA_SPEED * 32.0f) / OBJECT_GRID_SIZE;

	eye.x = halfw * sinf(t * 2);
	eye.y = totalheight + 8.0f;
	eye.z = -halfd * cosf(t * 3);

	tangent.x = halfw * cosf(t * 2) * 2;
	tangent.z = halfd * sinf(t * 3) * 3;
	tangent.y = sqrtf(tangent.x * tangent.x + tangent.z * tangent.z) * -tanf(Math::DegreesToRadians(60));

	up = { 0, 1, 0 };

	Math::Vec3Normalize(tangent, tangent);
	Math::Vec3Add(look, eye, tangent);

	Math::MatrixLookAtRH(view, eye, look, up);
	Math::MatrixPerspectiveFovRH(proj, Math::DegreesToRadians(60), (float)screenwidth / (float)screenheight, 1.5f, 50.0f);
	Math::MatrixMultiply(uniforms.drawbatchuniforms.viewproj, view, proj);

	time += elapsedtime;

	Math::Vector4 lightpos = { 1, 0.4f, 0.2f, 1 };
	float radius = sqrtf(totalwidth * totalwidth * 0.25f + totalheight * totalheight * 0.25f + totaldepth * totaldepth * 0.25f);

	Math::Vec3Normalize((Math::Vector3&)lightpos, (const Math::Vector3&)lightpos);
	Math::Vec3Scale((Math::Vector3&)lightpos, (const Math::Vector3&)lightpos, radius * 1.5f);

	uniforms.drawbatchuniforms.lightpos = lightpos;
	uniforms.drawbatchuniforms.eyepos = Math::Vector4(eye, 1);

	// update tiles
	UpdateTiles(uniforms.drawbatchuniforms.viewproj, currentdrawable);

	if (debugmode) {
		debugcamera.Animate(alpha);
		debugcamera.GetViewMatrix(view);
		debugcamera.GetProjectionMatrix(proj);

		Math::MatrixInverse(debugworld, uniforms.drawbatchuniforms.viewproj);
		Math::MatrixMultiply(uniforms.drawbatchuniforms.viewproj, view, proj);
		
		uniforms.debuguniforms.viewproj = uniforms.drawbatchuniforms.viewproj;
		uniforms.debuguniforms.color = Math::Color(1, 1, 1, 1);
	}

	// begin recording
	begininfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.pNext				= NULL;
	begininfo.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begininfo.pInheritanceInfo	= NULL;

	res = vkBeginCommandBuffer(primarycmdbuff, &begininfo);
	VK_ASSERT(res == VK_SUCCESS);

	// prepare for rendering
	VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	{
		barrier.ImageLayoutTransition(swapchainimg, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(depthbuffer, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	// upload uniform data
	barrier.Reset(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	{
		barrier.BufferAccessBarrier(uniformbuffer->GetBuffer(), VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
	}
	barrier.Enlist(primarycmdbuff);

	vkCmdUpdateBuffer(primarycmdbuff, uniformbuffer->GetBuffer(), 0, sizeof(CombinedUniformData), (const uint32_t*)&uniforms);

	barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
	{
		barrier.BufferAccessBarrier(uniformbuffer->GetBuffer(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	}
	barrier.Enlist(primarycmdbuff);

	// main render pass
	mainrenderpass->Begin(primarycmdbuff, mainframebuffers[currentdrawable], VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, Math::Color(0.017f, 0, 0, 1), 1.0f, 0);
	{
		std::vector<VkCommandBuffer> secondaries(visibletiles.size(), 0);

		for (size_t i = 0; i < visibletiles.size(); ++i)
			secondaries[i] = visibletiles[i]->SetForRender(presenter->GetCurrentFrame(), currentdrawable);

		// only valid command here
		vkCmdExecuteCommands(primarycmdbuff, (uint32_t)secondaries.size(), secondaries.data());

		mainrenderpass->NextSubpass(primarycmdbuff, VK_SUBPASS_CONTENTS_INLINE);

		if (debugmode) {
			Math::AABox		tilebox;
			Math::Matrix	zscale;
			Math::Color		debugcolor(0, 1, 1, 1);
			Math::Vector3	center, halfsize;
			float			tilewidth = totalwidth / TILE_GRID_SIZE;
			float			tiledepth = totaldepth / TILE_GRID_SIZE;

			// scale z from [-1, 1] to [0, 1]
			Math::MatrixIdentity(zscale);
			zscale._33 = zscale._43 = 0.5f;

			Math::MatrixMultiply(debugworld, zscale, debugworld);

			vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, simplecolor->GetPipeline());
			vkCmdBindDescriptorSets(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, simplecolor->GetPipelineLayout(), 0, 1, simplecolor->GetDescriptorSets(0), 0, NULL);
			vkCmdSetViewport(primarycmdbuff, 0, 1, simplecolor->GetViewport());
			vkCmdSetScissor(primarycmdbuff, 0, 1, simplecolor->GetScissor());

			// draw frustum
			vkCmdPushConstants(primarycmdbuff, simplecolor->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 64, debugworld);
			vkCmdPushConstants(primarycmdbuff, simplecolor->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, 16, debugcolor);

			debugmesh->Draw(primarycmdbuff, false);

			// draw grid
			for (size_t i = 0, j = 0; i < tiles.size() && j < visibletiles.size(); ++i) {
				bool visible = (visibletiles[j] == tiles[i]);
				bool marked = (tiles[i]->IsMarkedForDiscard() && !tiles[i]->IsDiscarded(currentdrawable));

				if (marked)
					debugcolor = Math::Color(1, 0, 0, 1);
				else
					debugcolor = Math::Color(1, 1, 1, 1);

				if (visible || marked) {
					size_t y = i / TILE_GRID_SIZE;
					size_t x = i % TILE_GRID_SIZE;

					tilebox.Min = Math::Vector3(totalwidth * -0.5f + x * tilewidth, 0, totaldepth * -0.5f + y * tiledepth);
					tilebox.Max = Math::Vector3(totalwidth * -0.5f + (x + 1) * tilewidth, totalheight, totaldepth * -0.5f + (y + 1) * tiledepth);

					tilebox.GetCenter(center);
					tilebox.GetHalfSize(halfsize);

					Math::MatrixScaling(debugworld, halfsize[0], halfsize[1], halfsize[2]);

					debugworld._41 = center[0];
					debugworld._42 = center[1];
					debugworld._43 = center[2];

					vkCmdPushConstants(primarycmdbuff, simplecolor->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 64, debugworld);
					vkCmdPushConstants(primarycmdbuff, simplecolor->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 64, 16, debugcolor);

					debugmesh->Draw(primarycmdbuff, false);
				}

				if (visible)
					++j;
			}
		}

		// render help text
		screenquad->GetPipeline()->SetViewport(10.0f, 10.0f, 512.0f, 512.0f);
		screenquad->GetPipeline()->SetScissor(10, 10, 512, 512);

		screenquad->Draw(primarycmdbuff);
	}
	mainrenderpass->End(primarycmdbuff);

	presenter->Present();
}

void FrameFinished(uint32_t frameid)
{
	for (size_t i = 0; i < tiles.size(); ++i) {
		tiles[i]->Discard(frameid);
	}
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIVulkan)) {
		delete app;
		return 1;
	}

	device = (VkDevice)app->GetLogicalDevice();
	presenter = (VulkanPresentationEngine*)app->GetSwapChain();

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
