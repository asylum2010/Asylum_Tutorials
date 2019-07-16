
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
#include "..\Common\geometryutils.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 72: Vulkan subgroup operations"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct UniformData
{
	Math::Matrix world;
	Math::Matrix worldinv;
	Math::Matrix viewproj;
	Math::Vector4 lightpos;
	Math::Vector4 eyepos;
};

// sample variables
Application*				app			= nullptr;
VkDevice					device		= nullptr;

VulkanPresentationEngine*	presenter	= nullptr;
VulkanMesh*					mesh		= nullptr;
VulkanGraphicsPipeline*		blinnphong	= nullptr;
VulkanComputePipeline*		coloredgrid	= nullptr;
VulkanImage*				texture		= nullptr;

BasicCamera					camera;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (driverInfo.subgroupProps.subgroupSize < 1) {
		MYERROR("InitScene(): This device doesn't support subgroup operations");
		return false;
	}

	mesh = new VulkanMesh(24, 36, sizeof(GeometryUtils::CommonVertex), nullptr, 0, VK_MESH_32BIT);
	VK_ASSERT(mesh != nullptr);

	// NOTE: meshes are always persistently mapped
	GeometryUtils::CommonVertex* vdata = (GeometryUtils::CommonVertex*)mesh->GetVertexBufferPointer();
	uint32_t* idata = (uint32_t*)mesh->GetIndexBufferPointer();

	GeometryUtils::CreateBox(vdata, idata, 1, 1, 1);

	// create texture
	texture = VulkanImage::Create2D(VK_FORMAT_R8G8B8A8_UNORM, 128, 128, 1, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);

	// upload to device
	VulkanTemporaryCommandBuffer(true, true, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		{
			// NOTE: also puts textures in SHADER_READ_ONLY_OPTIMAL layout
			mesh->UploadToVRAM(barrier, transfercmd);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			barrier.ImageLayoutTransition(texture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 0, 0);
		}
		barrier.Enlist(transfercmd);
		
		return true;
	});

	mesh->DeleteStagingBuffers();
	texture->DeleteStagingBuffer();

	// create pipeline for mesh
	blinnphong = new VulkanGraphicsPipeline();

	blinnphong->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "../../Media/ShadersVK/blinnphong.vert");
	blinnphong->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "../../Media/ShadersVK/blinnphong.frag");

	// setup descriptor set
	blinnphong->SetDescriptorSetLayoutBufferBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	blinnphong->SetDescriptorSetLayoutImageBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	blinnphong->AllocateDescriptorSets(1);

	blinnphong->SetDescriptorSetGroupBufferInfo(0, 0, mesh->GetUniformBufferInfo());
	blinnphong->SetDescriptorSetGroupImageInfo(0, 1, texture->GetImageInfo());

	blinnphong->UpdateDescriptorSet(0, 0);

	// input assembler state
	blinnphong->SetInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	blinnphong->SetInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12);
	blinnphong->SetInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, 24);

	blinnphong->SetVertexInputBinding(0, VulkanMakeBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(GeometryUtils::CommonVertex)));
	blinnphong->SetInputAssemblerState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	// other states
	blinnphong->SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	blinnphong->SetViewport(0, 0, (float)screenwidth, (float)screenheight);
	blinnphong->SetScissor(0, 0, screenwidth, screenheight);

	VK_ASSERT(blinnphong->Assemble(presenter->GetRenderPass(), 0));

	// create compute pipeline
	coloredgrid = new VulkanComputePipeline();

	coloredgrid->AddShader(VK_SHADER_STAGE_COMPUTE_BIT, "../../Media/ShadersVK/coloredgrid.comp");
	coloredgrid->AddPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float));

	coloredgrid->SetDescriptorSetLayoutImageBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	coloredgrid->AllocateDescriptorSets(1);
	coloredgrid->SetDescriptorSetGroupImageInfo(0, 0, texture->GetImageInfo());
	coloredgrid->UpdateDescriptorSet(0, 0);

	VK_ASSERT(coloredgrid->Assemble());

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetDistance(2.5f);

	return true;
}

void UninitScene()
{
	vkQueueWaitIdle(driverInfo.graphicsQueue);

	delete mesh;
	delete blinnphong;
	delete coloredgrid;

	VK_SAFE_RELEASE(texture);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		// TODO:
	}
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	Math::Matrix				world, view, proj;
	Math::Matrix				viewproj, worldinv, tmp;
	Math::Vector3				eye;

	uint32_t					currentdrawable	= presenter->GetNextDrawable();
	VkCommandBufferBeginInfo	begininfo		= {};
	VkRenderPassBeginInfo		passbegininfo	= {};
	VkClearValue				clearcolors[2];
	VkCommandBuffer				primarycmdbuff	= presenter->GetCommandBuffer(currentdrawable);
	VkImage						swapchainimg	= presenter->GetSwapchainImage(currentdrawable);
	VkImage						depthbuffer		= presenter->GetDepthBuffer()->GetImage();
	VkResult					res;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);

	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 1, 0, 0);
	Math::MatrixRotationAxis(world, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 0, 1, 0);
	Math::MatrixMultiply(world, tmp, world);
	Math::MatrixInverse(worldinv, world);

	time += elapsedtime;

	// update uniforms (they get fetched from host memory)
	UniformData* uniforms = (UniformData*)mesh->GetUniformBufferPointer();

	uniforms->world		= world;
	uniforms->worldinv	= worldinv;
	uniforms->viewproj	= viewproj;
	uniforms->lightpos	= Math::Vector4(6, 3, 10, 1);
	uniforms->eyepos	= Math::Vector4(eye, 1);

	// begin recording
	begininfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.pNext				= NULL;
	begininfo.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begininfo.pInheritanceInfo	= NULL;

	res = vkBeginCommandBuffer(primarycmdbuff, &begininfo);
	VK_ASSERT(res == VK_SUCCESS);

	// generate texture
	VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	{
		barrier.ImageLayoutTransition(texture, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 0, 0);
	}
	barrier.Enlist(primarycmdbuff);

	vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_COMPUTE, coloredgrid->GetPipeline());
	vkCmdBindDescriptorSets(primarycmdbuff, VK_PIPELINE_BIND_POINT_COMPUTE, coloredgrid->GetPipelineLayout(), 0, 1, coloredgrid->GetDescriptorSets(0), 0, NULL);

	vkCmdPushConstants(primarycmdbuff, coloredgrid->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &time);
	vkCmdDispatch(primarycmdbuff, 8, 8, 1);

	barrier.Reset(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	{
		barrier.ImageLayoutTransition(texture, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 0, 0);
	}
	barrier.Enlist(primarycmdbuff);

	// prepare for rendering
	barrier.Reset(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	{
		barrier.ImageLayoutTransition(swapchainimg, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
		barrier.ImageLayoutTransition(depthbuffer, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, 1, 0, 1);
	}
	barrier.Enlist(primarycmdbuff);

	// default render pass
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	clearcolors[0].color.float32[0] = 0.017f;
	clearcolors[0].color.float32[1] = 0;
	clearcolors[0].color.float32[2] = 0;
	clearcolors[0].color.float32[3] = 1;

	clearcolors[1].depthStencil.depth = 1.0f;
	clearcolors[1].depthStencil.stencil = 0;

	passbegininfo.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passbegininfo.pNext						= NULL;
	passbegininfo.renderPass				= presenter->GetRenderPass();
	passbegininfo.framebuffer				= presenter->GetFramebuffer(currentdrawable);
	passbegininfo.renderArea.offset.x		= 0;
	passbegininfo.renderArea.offset.y		= 0;
	passbegininfo.renderArea.extent.width	= screenwidth;
	passbegininfo.renderArea.extent.height	= screenheight;
	passbegininfo.clearValueCount			= 2;
	passbegininfo.pClearValues				= clearcolors;

	vkCmdBeginRenderPass(primarycmdbuff, &passbegininfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		// render mesh
		vkCmdBindPipeline(primarycmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, blinnphong->GetPipeline());
		vkCmdSetViewport(primarycmdbuff, 0, 1, blinnphong->GetViewport());
		vkCmdSetScissor(primarycmdbuff, 0, 1, blinnphong->GetScissor());

		mesh->Draw(primarycmdbuff, blinnphong);
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
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;

	app->Run();
	delete app;

	return 0;
}
