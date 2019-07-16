
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
#include "..\Common\spectatorcamera.h"

#define METERS_PER_UNIT		0.01f
#define NUM_LIGHTS			512
#define LIGHT_RADIUS		2.0f

// helper macros
#define TITLE				"Shader sample 71: Tile-based deferred rendering"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct CombinedUniformData
{
	// byte offset 0
	struct GBufferPassUniformData {
		Math::Matrix world;
		Math::Matrix worldinv;
		Math::Matrix viewproj;
	} gbufferuniforms;	// 192 B

	Math::Vector4 padding1[4];	// 64 B

	// byte offset 256
	struct AccumPassUniformData {
		Math::Matrix view;
		Math::Matrix proj;
		Math::Matrix viewproj;
		Math::Matrix viewprojinv;
		Math::Vector4 eyepos;
		Math::Vector4 clip;
	} accumuniforms;	// 288 B

	Math::Vector4 padding2[14];	// 224 B

	// byte offset 768
	struct ForwardPassUniformData {
		Math::Matrix world;
		Math::Matrix viewproj;
	} forwarduniforms;	// 128 B

	Math::Vector4 padding3[8];	// 128 B

	// byte offset 1024
	struct FlaresPassUniformData {
		Math::Matrix view;
		Math::Matrix proj;
		Math::Vector4 params;
	} flaresuniforms; // 144 B

	Math::Vector4 padding4[7];	// 112 B
};	// 1280 B

static_assert(sizeof(CombinedUniformData) % 256 == 0, "sizeof(CombinedUniformData) is not a multiple of 256");

struct LightParticle
{
	Math::Color	color;
	Math::Vector4 previous;
	Math::Vector4 current;
	Math::Vector3 velocity;
	float radius;
};

// sample variables
Application*				app					= nullptr;
VkDevice					device				= nullptr;
VkFramebuffer*				mainframebuffers	= nullptr;
VkFramebuffer				gbuffer				= nullptr;

VulkanRenderPass*			gbufferrenderpass	= nullptr;
VulkanImage*				gbuffernormals		= nullptr;
VulkanImage*				gbufferdepth		= nullptr;
VulkanGraphicsPipeline*		gbufferpasspipeline	= nullptr;

VulkanImage*				accumdiffirrad		= nullptr;
VulkanImage*				accumspecirrad		= nullptr;
VulkanComputePipeline*		accumpasspipeline	= nullptr;

VulkanRenderPass*			mainrenderpass		= nullptr;
VulkanGraphicsPipeline*		forwardpasspipeline	= nullptr;

VulkanImage*				tonemapinput		= nullptr;
VulkanGraphicsPipeline*		tonemappasspipeline	= nullptr;

VulkanGraphicsPipeline*		flarespasspipeline	= nullptr;

VulkanMesh*					model				= nullptr;
VulkanBuffer*				uniformbuffer		= nullptr;
VulkanBuffer*				lightbuffer			= nullptr;

VulkanImage*				flaretexture		= nullptr;
VulkanImage*				supplytexture		= nullptr;
VulkanImage*				supplynormalmap		= nullptr;
VulkanImage*				ambientcube			= nullptr;

VulkanPresentationEngine*	presenter			= nullptr;
VulkanScreenQuad*			screenquad			= nullptr;
VulkanImage*				helptext			= nullptr;

Math::AABox					particlespace(-13.2f, 2.0f, -5.7f, 13.2f, 14.4f, 5.7f);

SpectatorCamera				camera;
uint32_t					currentphysicsframe	= 0;
bool						drawtext			= true;
bool						freezetime			= false;

void InitializeGBufferPass();
void InitializeAccumPass();
void InitializeForwardPass();
void InitializeTonemapPass();
void InitializeFlaresPass();

void GenerateParticles();
void UpdateParticles(float dt);

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	// load model
	model = VulkanMesh::LoadFromQM("../../Media/MeshesQM/sponza/sponza.qm");
	VK_ASSERT(model != nullptr);

	std::cout << "Generating tangent frame...\n";

	model->GenerateTangentFrame();
	model->EnableSubset(4, false);

	// create combined uniform ringbuffer
	uniformbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanPresentationEngine::NUM_QUEUED_FRAMES * sizeof(CombinedUniformData), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// create other textures
	ambientcube = VulkanImage::CreateFromDDS("../../Media/Textures/uffizi_diff_irrad.dds", false);
	VK_ASSERT(ambientcube != nullptr);

	flaretexture = VulkanImage::CreateFromFile("../../Media/Textures/flare1.png", true);
	VK_ASSERT(flaretexture != nullptr);

	supplytexture = VulkanImage::CreateFromFile("../../Media/Textures/vk_logo.jpg", true);
	VK_ASSERT(supplytexture != nullptr);

	supplynormalmap = VulkanImage::Create2D(VK_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VK_ASSERT(supplynormalmap != nullptr);

	uint32_t* data = (uint32_t*)supplynormalmap->MapContents(0, 0);
	{
		*data = 0xff7f7fff;	// (0.5f, 0.5f, 1)
	}
	supplynormalmap->UnmapContents();

	// help text
	helptext = VulkanImage::Create2D(VK_FORMAT_B8G8R8A8_UNORM, 512, 512, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
	VulkanRenderText("Use WASD and mouse to move around\n\nT - Freeze time\nH - Toggle help text", helptext);

	// upload to device
	VulkanTemporaryCommandBuffer(true, true, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		{
			// NOTE: also puts textures in SHADER_READ_ONLY_OPTIMAL layout
			model->UploadToVRAM(barrier, transfercmd);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			
			ambientcube->UploadToVRAM(barrier, transfercmd);
			flaretexture->UploadToVRAM(barrier, transfercmd);
			supplytexture->UploadToVRAM(barrier, transfercmd);
			supplynormalmap->UploadToVRAM(barrier, transfercmd, false);
			helptext->UploadToVRAM(barrier, transfercmd, false);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			barrier.ImageLayoutTransition(ambientcube, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 0, 0);
			barrier.ImageLayoutTransition(flaretexture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 0, 0);
			barrier.ImageLayoutTransition(supplytexture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 0, 0);
			barrier.ImageLayoutTransition(supplynormalmap, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 0, 0);
			barrier.ImageLayoutTransition(helptext, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 0, 0);
		}
		barrier.Enlist(transfercmd);
		
		return true;
	});

	model->DeleteStagingBuffers();

	ambientcube->DeleteStagingBuffer();
	flaretexture->DeleteStagingBuffer();
	supplytexture->DeleteStagingBuffer();
	supplynormalmap->DeleteStagingBuffer();
	helptext->DeleteStagingBuffer();

	// generate particles
	GenerateParticles();

	// create main (forward) render pass
	mainrenderpass = new VulkanRenderPass(screenwidth, screenheight, 2);

	mainrenderpass->AddAttachment(driverInfo.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	mainrenderpass->AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	mainrenderpass->AddAttachment(presenter->GetDepthBuffer()->GetFormat(), VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// forward pass renders into attachment 1
	mainrenderpass->AddSubpassInputReference(0, 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	mainrenderpass->AddSubpassInputReference(0, 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	mainrenderpass->AddSubpassColorReference(0, 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->SetSubpassDepthReference(0, 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	mainrenderpass->AddSubpassPreserveReference(0, 0);

	// tonemap pass renders into attachment 0
	mainrenderpass->AddSubpassInputReference(1, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	mainrenderpass->AddSubpassColorReference(1, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mainrenderpass->SetSubpassDepthReference(1, 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// external dependency adds a barrier between g-buffer pass and forward pass
	mainrenderpass->AddDependency(
		VK_SUBPASS_EXTERNAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	mainrenderpass->AddDependency(
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		1,
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	VK_ASSERT(mainrenderpass->Assemble());

	screenquad = new VulkanScreenQuad(mainrenderpass->GetRenderPass(), 1, helptext);

	// initialize other render passes
	InitializeGBufferPass();
	InitializeAccumPass();
	InitializeForwardPass();
	InitializeTonemapPass();
	InitializeFlaresPass();

	// create g-buffer framebuffer
	VkImageView				fbattachments[5];
	VkFramebufferCreateInfo	framebufferinfo = {};
	VkResult				res;

	framebufferinfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferinfo.pNext			= NULL;
	framebufferinfo.renderPass		= gbufferrenderpass->GetRenderPass();
	framebufferinfo.attachmentCount	= 3;
	framebufferinfo.pAttachments	= fbattachments;
	framebufferinfo.width			= screenwidth;
	framebufferinfo.height			= screenheight;
	framebufferinfo.layers			= 1;

	fbattachments[0] = gbuffernormals->GetImageView();
	fbattachments[1] = gbufferdepth->GetImageView();
	fbattachments[2] = presenter->GetDepthBuffer()->GetImageView();

	res = vkCreateFramebuffer(device, &framebufferinfo, NULL, &gbuffer);
	VK_ASSERT(res == VK_SUCCESS);

	// create main render pass framebuffers
	mainframebuffers = new VkFramebuffer[driverInfo.numSwapChainImages];

	framebufferinfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferinfo.pNext			= NULL;
	framebufferinfo.renderPass		= mainrenderpass->GetRenderPass();
	framebufferinfo.attachmentCount	= ARRAY_SIZE(fbattachments);
	framebufferinfo.pAttachments	= fbattachments;
	framebufferinfo.width			= screenwidth;
	framebufferinfo.height			= screenheight;
	framebufferinfo.layers			= 1;

	fbattachments[1] = tonemapinput->GetImageView();
	fbattachments[2] = accumdiffirrad->GetImageView();
	fbattachments[3] = accumspecirrad->GetImageView();
	fbattachments[4] = presenter->GetDepthBuffer()->GetImageView();

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		fbattachments[0] = driverInfo.swapchainImageViews[i];

		res = vkCreateFramebuffer(device, &framebufferinfo, NULL, &mainframebuffers[i]);
		VK_ASSERT(res == VK_SUCCESS);
	}

	// setup camera
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetClipPlanes(0.1f, 50.0f);

	camera.SetEyePosition(-0.12f, 3.0f, 0);
	//camera.SetEyePosition(-10.0f, 8.99f, -3.5f);
	camera.SetOrientation(-Math::HALF_PI, 0, 0);

	return true;
}

void InitializeGBufferPass()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	VkDescriptorBufferInfo gbufferuniforminfo;

	gbufferuniforminfo.buffer = uniformbuffer->GetBuffer();
	gbufferuniforminfo.offset = offsetof(CombinedUniformData, gbufferuniforms);
	gbufferuniforminfo.range = sizeof(CombinedUniformData::GBufferPassUniformData);

	// render targets & render pass
	gbuffernormals = VulkanImage::Create2D(VK_FORMAT_R16G16B16A16_SFLOAT, screenwidth, screenheight, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_STORAGE_BIT);
	gbufferdepth = VulkanImage::Create2D(VK_FORMAT_R32_SFLOAT, screenwidth, screenheight, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_STORAGE_BIT);

	gbufferrenderpass = new VulkanRenderPass(app->GetClientWidth(), app->GetClientHeight(), 1);

	gbufferrenderpass->AddAttachment(gbuffernormals->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	gbufferrenderpass->AddAttachment(gbufferdepth->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	gbufferrenderpass->AddAttachment(presenter->GetDepthBuffer()->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	gbufferrenderpass->AddSubpassColorReference(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gbufferrenderpass->AddSubpassColorReference(0, 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gbufferrenderpass->SetSubpassDepthReference(0, 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VK_ASSERT(gbufferrenderpass->Assemble());

	// pipeline
	gbufferpasspipeline = new VulkanGraphicsPipeline();

	VK_ASSERT(gbufferpasspipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/gbuffer.vert"));
	VK_ASSERT(gbufferpasspipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/gbuffer.frag"));

	gbufferpasspipeline->SetInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);	// position
	gbufferpasspipeline->SetInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12);	// normal
	gbufferpasspipeline->SetInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, 24);		// texcoord
	gbufferpasspipeline->SetInputAttribute(3, 0, VK_FORMAT_R32G32B32_SFLOAT, 32);	// tangent
	gbufferpasspipeline->SetInputAttribute(4, 0, VK_FORMAT_R32G32B32_SFLOAT, 44);	// bitangent

	gbufferpasspipeline->SetVertexInputBinding(0, VulkanMakeBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, model->GetVertexStride()));

	gbufferpasspipeline->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	gbufferpasspipeline->SetDescriptorSetLayoutImageBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	gbufferpasspipeline->AllocateDescriptorSets(model->GetNumSubsets());

	for (uint32_t i = 0; i < model->GetNumSubsets(); ++i) {
		const VulkanMaterial& material = model->GetMaterial(i);

		gbufferpasspipeline->SetDescriptorSetGroupBufferInfo(0, 0, &gbufferuniforminfo);

		if (material.normalMap != nullptr)
			gbufferpasspipeline->SetDescriptorSetGroupImageInfo(0, 1, material.normalMap->GetImageInfo());
		else
			gbufferpasspipeline->SetDescriptorSetGroupImageInfo(0, 1, supplynormalmap->GetImageInfo());

		gbufferpasspipeline->UpdateDescriptorSet(0, i);
	}

	gbufferpasspipeline->SetBlendState(0, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO);
	gbufferpasspipeline->SetBlendState(1, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO);

	gbufferpasspipeline->SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	gbufferpasspipeline->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	gbufferpasspipeline->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(gbufferpasspipeline->Assemble(gbufferrenderpass->GetRenderPass(), 0));
}

void InitializeAccumPass()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	VkDescriptorBufferInfo accumuniforminfo;

	accumuniforminfo.buffer = uniformbuffer->GetBuffer();
	accumuniforminfo.offset = offsetof(CombinedUniformData, accumuniforms);
	accumuniforminfo.range = sizeof(CombinedUniformData::AccumPassUniformData);

	// storage images
	accumdiffirrad = VulkanImage::Create2D(VK_FORMAT_R16G16B16A16_SFLOAT, screenwidth, screenheight, 1, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	accumspecirrad = VulkanImage::Create2D(VK_FORMAT_R16G16B16A16_SFLOAT, screenwidth, screenheight, 1, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

	VK_ASSERT(accumdiffirrad);
	VK_ASSERT(accumspecirrad);

	// pipeline
	VulkanSpecializationInfo specinfo;

	specinfo.AddUInt(1, NUM_LIGHTS);

	accumpasspipeline = new VulkanComputePipeline();
	VK_ASSERT(accumpasspipeline->AddShader(VK_SHADER_STAGE_COMPUTE_BIT, "../../Media/ShadersVK/deferredaccum.comp", specinfo));

	accumpasspipeline->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutImageBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutImageBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutImageBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutImageBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutImageBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	accumpasspipeline->SetDescriptorSetLayoutBufferBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_COMPUTE_BIT);

	VkDescriptorImageInfo imginfo1 = *gbuffernormals->GetImageInfo();
	VkDescriptorImageInfo imginfo2 = *gbufferdepth->GetImageInfo();
	VkDescriptorImageInfo imginfo3 = *accumdiffirrad->GetImageInfo();
	VkDescriptorImageInfo imginfo4 = *accumspecirrad->GetImageInfo();
	VkDescriptorImageInfo imginfo5 = *ambientcube->GetImageInfo();

	imginfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imginfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imginfo3.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imginfo4.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imginfo5.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo buffinfo1 = lightbuffer->GetBufferInfo();
	buffinfo1.range = NUM_LIGHTS * sizeof(LightParticle);

	accumpasspipeline->AllocateDescriptorSets(1);
	{
		accumpasspipeline->SetDescriptorSetGroupBufferInfo(0, 0, &accumuniforminfo);
		accumpasspipeline->SetDescriptorSetGroupImageInfo(0, 1, &imginfo1);
		accumpasspipeline->SetDescriptorSetGroupImageInfo(0, 2, &imginfo2);
		accumpasspipeline->SetDescriptorSetGroupImageInfo(0, 3, &imginfo3);
		accumpasspipeline->SetDescriptorSetGroupImageInfo(0, 4, &imginfo4);
		accumpasspipeline->SetDescriptorSetGroupImageInfo(0, 5, &imginfo5);
		accumpasspipeline->SetDescriptorSetGroupBufferInfo(0, 6, &buffinfo1);
	}
	accumpasspipeline->UpdateDescriptorSet(0, 0);

	VK_ASSERT(accumpasspipeline->Assemble());
}

void InitializeForwardPass()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	VkDescriptorBufferInfo forwarduniforminfo;

	forwarduniforminfo.buffer = uniformbuffer->GetBuffer();
	forwarduniforminfo.offset = offsetof(CombinedUniformData, forwarduniforms);
	forwarduniforminfo.range = sizeof(CombinedUniformData::ForwardPassUniformData);

	forwardpasspipeline = new VulkanGraphicsPipeline();

	VK_ASSERT(forwardpasspipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/forward.vert"));
	VK_ASSERT(forwardpasspipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/forward.frag"));

	forwardpasspipeline->SetInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);	// position
	forwardpasspipeline->SetInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12);	// normal
	forwardpasspipeline->SetInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, 24);		// texcoord

	forwardpasspipeline->SetVertexInputBinding(0, VulkanMakeBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, model->GetVertexStride()));
	
	forwardpasspipeline->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	forwardpasspipeline->SetDescriptorSetLayoutImageBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	forwardpasspipeline->SetDescriptorSetLayoutImageBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
	forwardpasspipeline->SetDescriptorSetLayoutImageBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorImageInfo imginfo1 = *accumdiffirrad->GetImageInfo();
	VkDescriptorImageInfo imginfo2 = *accumspecirrad->GetImageInfo();

	imginfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imginfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	forwardpasspipeline->AllocateDescriptorSets(model->GetNumSubsets());

	for (uint32_t i = 0; i < model->GetNumSubsets(); ++i) {
		const VulkanMaterial& material = model->GetMaterial(i);

		forwardpasspipeline->SetDescriptorSetGroupBufferInfo(0, 0, &forwarduniforminfo);

		if (material.texture != nullptr)
			forwardpasspipeline->SetDescriptorSetGroupImageInfo(0, 1, material.texture->GetImageInfo());
		else
			forwardpasspipeline->SetDescriptorSetGroupImageInfo(0, 1, supplytexture->GetImageInfo());

		forwardpasspipeline->SetDescriptorSetGroupImageInfo(0, 2, &imginfo1);
		forwardpasspipeline->SetDescriptorSetGroupImageInfo(0, 3, &imginfo2);

		forwardpasspipeline->UpdateDescriptorSet(0, i);
	}

	forwardpasspipeline->SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
	forwardpasspipeline->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	forwardpasspipeline->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(forwardpasspipeline->Assemble(mainrenderpass->GetRenderPass(), 0));
}

void InitializeTonemapPass()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	//tonemapinput = VulkanImage::CreateAlias(gbuffernormals);
	tonemapinput = VulkanImage::Create2D(VK_FORMAT_R16G16B16A16_SFLOAT, screenwidth, screenheight, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	VK_ASSERT(tonemapinput);

	tonemappasspipeline = new VulkanGraphicsPipeline();

	VK_ASSERT(tonemappasspipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/screenquad.vert"));
	VK_ASSERT(tonemappasspipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/tonemap.frag"));

	tonemappasspipeline->SetDescriptorSetLayoutImageBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
	tonemappasspipeline->AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, 64);

	VkDescriptorImageInfo imginfo1 = *tonemapinput->GetImageInfo();
	imginfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	tonemappasspipeline->AllocateDescriptorSets(1);
	tonemappasspipeline->SetDescriptorSetGroupImageInfo(0, 0, &imginfo1);
	tonemappasspipeline->UpdateDescriptorSet(0, 0);

	tonemappasspipeline->SetInputAssemblerState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
	tonemappasspipeline->SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
	tonemappasspipeline->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	tonemappasspipeline->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(tonemappasspipeline->Assemble(mainrenderpass->GetRenderPass(), 1));
}

void InitializeFlaresPass()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	VkDescriptorBufferInfo flaresuniforminfo;

	flaresuniforminfo.buffer = uniformbuffer->GetBuffer();
	flaresuniforminfo.offset = offsetof(CombinedUniformData, flaresuniforms);
	flaresuniforminfo.range = sizeof(CombinedUniformData::FlaresPassUniformData);

	flarespasspipeline = new VulkanGraphicsPipeline();

	VK_ASSERT(flarespasspipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/flares.vert"));
	VK_ASSERT(flarespasspipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/flares.frag"));

	flarespasspipeline->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	flarespasspipeline->SetDescriptorSetLayoutBufferBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
	flarespasspipeline->SetDescriptorSetLayoutImageBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorImageInfo imginfo1 = *flaretexture->GetImageInfo();
	imginfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	VkDescriptorBufferInfo buffinfo1 = lightbuffer->GetBufferInfo();
	buffinfo1.range = NUM_LIGHTS * sizeof(LightParticle);

	flarespasspipeline->AllocateDescriptorSets(1);
	flarespasspipeline->SetDescriptorSetGroupBufferInfo(0, 0, &flaresuniforminfo);
	flarespasspipeline->SetDescriptorSetGroupBufferInfo(0, 1, &buffinfo1);
	flarespasspipeline->SetDescriptorSetGroupImageInfo(0, 2, &imginfo1);
	flarespasspipeline->UpdateDescriptorSet(0, 0);

	flarespasspipeline->SetInputAssemblerState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
	flarespasspipeline->SetBlendState(0, VK_TRUE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
	flarespasspipeline->SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS);
	flarespasspipeline->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	flarespasspipeline->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(flarespasspipeline->Assemble(mainrenderpass->GetRenderPass(), 1));
}

void UninitScene()
{
	vkQueueWaitIdle(driverInfo.graphicsQueue);

	VK_SAFE_RELEASE(gbuffernormals);
	VK_SAFE_RELEASE(gbufferdepth);

	delete gbufferrenderpass;
	delete gbufferpasspipeline;

	VK_SAFE_RELEASE(accumdiffirrad);
	VK_SAFE_RELEASE(accumspecirrad);

	delete accumpasspipeline;
	delete forwardpasspipeline;
	delete tonemappasspipeline;
	delete flarespasspipeline;

	VK_SAFE_RELEASE(flaretexture);
	VK_SAFE_RELEASE(ambientcube);
	VK_SAFE_RELEASE(supplynormalmap);
	VK_SAFE_RELEASE(supplytexture);
	VK_SAFE_RELEASE(tonemapinput);

	if (gbuffer)
		vkDestroyFramebuffer(device, gbuffer, NULL);

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		if (mainframebuffers[i])
			vkDestroyFramebuffer(device, mainframebuffers[i], NULL);
	}

	delete[] mainframebuffers;

	delete mainrenderpass;
	delete model;
	delete uniformbuffer;
	delete lightbuffer;
	delete screenquad;

	VK_SAFE_RELEASE(helptext);
}

void GenerateParticles()
{
	static Math::Color colors[] = {
		Math::Color(1, 0, 0, 1),
		Math::Color(0, 1, 0, 1),
		Math::Color(0, 0, 1, 1),

		Math::Color(1, 0, 1, 1),
		Math::Color(0, 1, 1, 1),
		Math::Color(1, 1, 0, 1),
	};

	lightbuffer = VulkanBuffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanPresentationEngine::NUM_QUEUED_FRAMES * NUM_LIGHTS * sizeof(LightParticle), VK_MEMORY_PROPERTY_SHARED_BIT);
	VK_ASSERT(lightbuffer != NULL);

	LightParticle* particles = (LightParticle*)lightbuffer->MapContents(0, 0);

	for (int i = 0; i < NUM_LIGHTS; ++i) {
		float x = Math::RandomFloat();
		float y = Math::RandomFloat();
		float z = Math::RandomFloat();
		float v = Math::RandomFloat();
		float d = ((rand() % 2) ? 1.0f : -1.0f);
		int c = rand() % ARRAY_SIZE(colors);

		x = particlespace.Min[0] + x * (particlespace.Max[0] - particlespace.Min[0]);
		y = particlespace.Min[1] + y * (particlespace.Max[1] - particlespace.Min[1]);
		z = particlespace.Min[2] + z * (particlespace.Max[2] - particlespace.Min[2]);
		v = 0.4f + v * (3.0f - 0.4f);

		for (int j = 0; j < VulkanPresentationEngine::NUM_QUEUED_FRAMES; ++j) {
			particles[i + j * NUM_LIGHTS].color	= colors[c];
			particles[i + j * NUM_LIGHTS].radius = LIGHT_RADIUS;

			particles[i + j * NUM_LIGHTS].previous = Math::Vector4(x, y, z, 1);
			particles[i + j * NUM_LIGHTS].current = Math::Vector4(x, y, z, 1);
			particles[i + j * NUM_LIGHTS].velocity = Math::Vector3(0, v * d, 0);
		}
	}

	lightbuffer->UnmapContents();

	// upload to GPU
	VulkanTemporaryCommandBuffer(true, false, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		lightbuffer->UploadToVRAM(transfercmd);

		barrier.BufferAccessBarrier(lightbuffer->GetBuffer(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
		barrier.Enlist(transfercmd);

		return true;
	});
}

void UpdateParticles(float dt)
{
	Math::Vector3	vx, vy, vz;
	Math::Vector3	b;
	Math::Matrix	A, Ainv;
	Math::Vector4	planes[6];

	Math::Vector4*	bestplane = nullptr;
	float			denom, energy;
	float			toi, besttoi;
	float			impulse, noise;
	bool			pastcollision;

	particlespace.GetPlanes(planes);

	uint32_t prevphysicsframe = (currentphysicsframe + VulkanPresentationEngine::NUM_QUEUED_FRAMES - 1) % VulkanPresentationEngine::NUM_QUEUED_FRAMES;

	LightParticle* particles = (LightParticle*)lightbuffer->MapContents(0, 0);
	LightParticle* readparticles = particles + prevphysicsframe * NUM_LIGHTS;
	LightParticle* writeparticles = particles + currentphysicsframe * NUM_LIGHTS;

	for (int i = 0; i < NUM_LIGHTS; ++i) {
		const LightParticle& oldp = readparticles[i];
		LightParticle& newp = writeparticles[i];

		newp.velocity = oldp.velocity;
		newp.previous = oldp.current;

		// integrate
		Math::Vec3Mad((Math::Vector3&)newp.current, (const Math::Vector3&)oldp.current, oldp.velocity, dt);

		// detect collision
		besttoi = 2;

		Math::Vec3Subtract(b, (const Math::Vector3&)newp.current, (const Math::Vector3&)newp.previous);

		for (int j = 0; j < 6; ++j) {
			// use radius == 0.5
			denom = Math::Vec3Dot(b, (const Math::Vector3&)planes[j]);
			pastcollision = (Math::Vec3Dot((const Math::Vector3&)newp.previous, (const Math::Vector3&)planes[j]) + planes[j].w < 0.5f);

			if (denom < -1e-4f) {
				toi = (0.5f - Math::Vec3Dot((const Math::Vector3&)newp.previous, (const Math::Vector3&)planes[j]) - planes[j].w) / denom;

				if (((toi <= 1 && toi >= 0) ||		// normal case
					(toi < 0 && pastcollision)) &&	// allow past collision
					toi < besttoi)
				{
					besttoi = toi;
					bestplane = &planes[j];
				}
			}
		}

		if (besttoi <= 1) {
			// resolve constraint
			newp.current[0] = (1 - besttoi) * newp.previous[0] + besttoi * newp.current[0];
			newp.current[1] = (1 - besttoi) * newp.previous[1] + besttoi * newp.current[1];
			newp.current[2] = (1 - besttoi) * newp.previous[2] + besttoi * newp.current[2];

			impulse = -Math::Vec3Dot((const Math::Vector3&)*bestplane, newp.velocity);

			// perturb normal vector
			noise = ((rand() % 100) / 100.0f) * Math::PI * 0.333333f - Math::PI * 0.166666f; // [-pi/6, pi/6]

			b[0] = cosf(noise + Math::PI * 0.5f);
			b[1] = cosf(noise);
			b[2] = 0;

			Math::Vec3Normalize(vy, (const Math::Vector3&)(*bestplane));
			Math::GetOrthogonalVectors(vx, vz, vy);

			A._11 = vx[0];	A._12 = vy[0];	A._13 = vz[0];	A._14 = 0;
			A._21 = vx[1];	A._22 = vy[1];	A._23 = vz[1];	A._24 = 0;
			A._31 = vx[2];	A._32 = vy[2];	A._33 = vz[2];	A._34 = 0;
			A._41 = 0;		A._42 = 0;		A._43 = 0;		A._44 = 1;

			Math::MatrixInverse(Ainv, A);
			Math::Vec3TransformNormal(vy, b, Ainv);

			energy = Math::Vec3Length(newp.velocity);

			newp.velocity[0] += 2 * impulse * vy[0];
			newp.velocity[1] += 2 * impulse * vy[1];
			newp.velocity[2] += 2 * impulse * vy[2];

			// must conserve energy
			Math::Vec3Normalize(newp.velocity, newp.velocity);

			newp.velocity[0] *= energy;
			newp.velocity[1] *= energy;
			newp.velocity[2] *= energy;
		}
	}

	lightbuffer->UnmapContents();

	// NOTE: Update() is called before render
	VulkanTemporaryCommandBuffer(true, false, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		barrier.BufferAccessBarrier(lightbuffer->GetBuffer(), VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, currentphysicsframe * NUM_LIGHTS, NUM_LIGHTS * sizeof(LightParticle));
		barrier.Enlist(transfercmd);

		lightbuffer->UploadToVRAM(transfercmd);

		barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		barrier.BufferAccessBarrier(lightbuffer->GetBuffer(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, currentphysicsframe * NUM_LIGHTS, NUM_LIGHTS * sizeof(LightParticle));
		barrier.Enlist(transfercmd);

		return true;
	});

	currentphysicsframe = (currentphysicsframe + 1) % VulkanPresentationEngine::NUM_QUEUED_FRAMES;
}

void KeyDown(KeyCode key)
{
	camera.OnKeyDown(key);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCodeH:
		drawtext = !drawtext;
		break;

	case KeyCodeT:
		freezetime = !freezetime;
		break;

	default:
		break;
	}

	camera.OnKeyUp(key);
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	camera.OnMouseMove(dx, dy);
}

void MouseDown(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseDown(button);
}

void MouseUp(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseUp(button);
}

void Update(float delta)
{
	UpdateParticles(freezetime ? 0 : delta);

	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;
	static float stopalpha = -1;

	CombinedUniformData			uniforms;
	Math::Matrix				world, view, proj;
	Math::Vector3				eye;

	uint32_t					currentdrawable	= presenter->GetNextDrawable();
	VkCommandBufferBeginInfo	begininfo		= {};
	VkRenderPassBeginInfo		passbegininfo	= {};
	VkClearValue				clearcolors[5];
	VkCommandBuffer				primarycmdbuff	= presenter->GetCommandBuffer(currentdrawable);
	VkImage						swapchainimg	= presenter->GetSwapchainImage(currentdrawable);
	VkImage						depthbuffer		= presenter->GetDepthBuffer()->GetImage();
	VkResult					res;

	if (freezetime) {
		if (stopalpha == -1)
			stopalpha = alpha;
	} else {
		stopalpha = -1;
	}

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	// g-buffer pass uniforms
	Math::MatrixMultiply(uniforms.gbufferuniforms.viewproj, view, proj);
	Math::MatrixScaling(uniforms.gbufferuniforms.world, METERS_PER_UNIT, METERS_PER_UNIT, METERS_PER_UNIT);

	uniforms.gbufferuniforms.world._41 = 60.518921f * METERS_PER_UNIT;
	uniforms.gbufferuniforms.world._42 = (778.0f - 651.495361f) * METERS_PER_UNIT;
	uniforms.gbufferuniforms.world._43 = -38.690552f * METERS_PER_UNIT;

	Math::MatrixInverse(uniforms.gbufferuniforms.worldinv, uniforms.gbufferuniforms.world);

	// accumulation pass uniforms
	float interpolator = (freezetime ? stopalpha : alpha);

	Math::MatrixInverse(uniforms.accumuniforms.viewprojinv, uniforms.gbufferuniforms.viewproj);
	
	uniforms.accumuniforms.viewproj = uniforms.gbufferuniforms.viewproj;
	uniforms.accumuniforms.proj = proj;
	uniforms.accumuniforms.view = view;

	uniforms.accumuniforms.eyepos = Math::Vector4(eye, 1);
	uniforms.accumuniforms.clip = Math::Vector4(camera.GetNearPlane(), camera.GetFarPlane(), interpolator, 0);

	// forward pass uniforms
	uniforms.forwarduniforms.world = uniforms.gbufferuniforms.world;
	uniforms.forwarduniforms.viewproj = uniforms.gbufferuniforms.viewproj;

	// flares pass uniforms
	uniforms.flaresuniforms.view = view;
	uniforms.flaresuniforms.proj = proj;
	uniforms.flaresuniforms.params = Math::Vector4(interpolator, time, 0, 0);

	if (!freezetime)
		time += elapsedtime;

	// begin recording
	begininfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.pNext				= NULL;
	begininfo.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begininfo.pInheritanceInfo	= NULL;

	res = vkBeginCommandBuffer(primarycmdbuff, &begininfo);
	VK_ASSERT(res == VK_SUCCESS);

	// upload uniform data
	VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
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

	// prepare for rendering
	barrier.Reset(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	{
		barrier.ImageLayoutTransition(swapchainimg, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(depthbuffer, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
	
		barrier.ImageLayoutTransition(gbuffernormals->GetImage(), 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(gbufferdepth->GetImage(), 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
		
		barrier.ImageLayoutTransition(tonemapinput->GetImage(), 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	barrier.Reset(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	{
		barrier.ImageLayoutTransition(accumdiffirrad->GetImage(), 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(accumspecirrad->GetImage(), 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	// g-buffer pass
	gbufferrenderpass->Begin(primarycmdbuff, gbuffer, VK_SUBPASS_CONTENTS_INLINE, Math::Color(0, 0, 0, 1), 1.0f, 0);
	{
		vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferpasspipeline->GetPipeline());
		vkCmdSetViewport(primarycmdbuff, 0, 1, gbufferpasspipeline->GetViewport());
		vkCmdSetScissor(primarycmdbuff, 0, 1, gbufferpasspipeline->GetScissor());

		model->Draw(primarycmdbuff, gbufferpasspipeline);
	}
	gbufferrenderpass->End(primarycmdbuff);

	// accumulation pass
	uint32_t prevphysicsframe = (currentphysicsframe + VulkanPresentationEngine::NUM_QUEUED_FRAMES - 1) % VulkanPresentationEngine::NUM_QUEUED_FRAMES;
	uint32_t lightbufferoffset = prevphysicsframe * NUM_LIGHTS * sizeof(LightParticle);

	vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_COMPUTE, accumpasspipeline->GetPipeline());
	vkCmdBindDescriptorSets(primarycmdbuff, VK_PIPELINE_BIND_POINT_COMPUTE, accumpasspipeline->GetPipelineLayout(), 0, 1, accumpasspipeline->GetDescriptorSets(0), 1, &lightbufferoffset);

	uint32_t screenwidth	= app->GetClientWidth();
	uint32_t screenheight	= app->GetClientHeight();
	uint32_t workgroupsx	= (screenwidth + (screenwidth % 16)) / 16;
	uint32_t workgroupsy	= (screenheight + (screenheight % 16)) / 16;

	barrier.Reset(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	{
		barrier.ImageLayoutTransition(gbuffernormals->GetImage(), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(gbufferdepth->GetImage(), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	vkCmdDispatch(primarycmdbuff, workgroupsx, workgroupsy, 1);

	barrier.Reset(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	{
		barrier.ImageLayoutTransition(accumdiffirrad->GetImage(), VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(accumspecirrad->GetImage(), VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	// main render pass
	clearcolors[0].color.float32[0] = 0.017f;
	clearcolors[0].color.float32[1] = 0;
	clearcolors[0].color.float32[2] = 0;
	clearcolors[0].color.float32[3] = 1;

	clearcolors[1].color.float32[0] = 0.017f;
	clearcolors[1].color.float32[1] = 0;
	clearcolors[1].color.float32[2] = 0;
	clearcolors[1].color.float32[3] = 1;

	clearcolors[2].color.float32[0] = 0.017f;
	clearcolors[2].color.float32[1] = 0;
	clearcolors[2].color.float32[2] = 0;
	clearcolors[2].color.float32[3] = 1;

	clearcolors[3].color.float32[0] = 0.017f;
	clearcolors[3].color.float32[1] = 0;
	clearcolors[3].color.float32[2] = 0;
	clearcolors[3].color.float32[3] = 1;

	clearcolors[4].depthStencil.depth = 1.0f;
	clearcolors[4].depthStencil.stencil = 0;

	passbegininfo.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passbegininfo.pNext						= NULL;
	passbegininfo.renderPass				= mainrenderpass->GetRenderPass();
	passbegininfo.framebuffer				= mainframebuffers[currentdrawable];
	passbegininfo.renderArea.offset.x		= 0;
	passbegininfo.renderArea.offset.y		= 0;
	passbegininfo.renderArea.extent.width	= screenwidth;
	passbegininfo.renderArea.extent.height	= screenheight;
	passbegininfo.clearValueCount			= 5;
	passbegininfo.pClearValues				= clearcolors;

	vkCmdBeginRenderPass(primarycmdbuff, &passbegininfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		// forward pass
		vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardpasspipeline->GetPipeline());
		vkCmdSetViewport(primarycmdbuff, 0, 1, forwardpasspipeline->GetViewport());
		vkCmdSetScissor(primarycmdbuff, 0, 1, forwardpasspipeline->GetScissor());

		model->Draw(primarycmdbuff, forwardpasspipeline);

		// tonemap pass
		vkCmdNextSubpass(primarycmdbuff, VK_SUBPASS_CONTENTS_INLINE);

		Math::MatrixIdentity(world);

		vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemappasspipeline->GetPipeline());
		vkCmdBindDescriptorSets(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemappasspipeline->GetPipelineLayout(), 0, 1, tonemappasspipeline->GetDescriptorSets(0), 0, NULL);
		vkCmdPushConstants(primarycmdbuff, tonemappasspipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &world);

		vkCmdSetViewport(primarycmdbuff, 0, 1, tonemappasspipeline->GetViewport());
		vkCmdSetScissor(primarycmdbuff, 0, 1, tonemappasspipeline->GetScissor());

		vkCmdDraw(primarycmdbuff, 4, 1, 0, 0);

		// flares pass
		vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, flarespasspipeline->GetPipeline());
		vkCmdBindDescriptorSets(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, flarespasspipeline->GetPipelineLayout(), 0, 1, flarespasspipeline->GetDescriptorSets(0), 1, &lightbufferoffset);
		vkCmdSetViewport(primarycmdbuff, 0, 1, flarespasspipeline->GetViewport());
		vkCmdSetScissor(primarycmdbuff, 0, 1, flarespasspipeline->GetScissor());

		vkCmdDraw(primarycmdbuff, 4, NUM_LIGHTS, 0, 0);

		if (drawtext) {
			// help text
			screenquad->GetPipeline()->SetViewport(10.0f, 10.0f, 512.0f, 512.0f);
			screenquad->GetPipeline()->SetScissor(10, 10, 512, 512);

			screenquad->Draw(primarycmdbuff);
		}
	}
	vkCmdEndRenderPass(primarycmdbuff);

	presenter->Present();
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
	app->KeyDownCallback = KeyDown;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;

	app->Run();
	delete app;

	return 0;
}
