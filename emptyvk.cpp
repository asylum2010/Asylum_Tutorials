
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

// helper macros
#define TITLE				"Shader sample XX: Title"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*				app			= nullptr;

VkDevice					device		= nullptr;

VulkanPresentationEngine*	presenter	= nullptr;
VulkanScreenQuad*			screenquad	= nullptr;
VulkanImage*				helptext	= nullptr;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	// TODO: initialize scene

	// help text
	helptext = VulkanImage::Create2D(VK_FORMAT_B8G8R8A8_UNORM, 512, 512, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);

	VulkanRenderText(
		"// TODO:",
		helptext);

	// upload to device
	VulkanTemporaryCommandBuffer(true, true, [&](VkCommandBuffer transfercmd) -> bool {
		VulkanPipelineBarrierBatch barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		{
			helptext->UploadToVRAM(barrier, transfercmd, false);

			barrier.Reset(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			barrier.ImageLayoutTransition(helptext, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
		}
		barrier.Enlist(transfercmd);
		
		return true;
	});

	helptext->DeleteStagingBuffer();

	screenquad = new VulkanScreenQuad(presenter->GetRenderPass(), 0, helptext);

	return true;
}

void UninitScene()
{
	vkQueueWaitIdle(driverInfo.graphicsQueue);

	delete screenquad;

	VK_SAFE_RELEASE(helptext);
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
	// TODO: update
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix				world;

	uint32_t					currentdrawable	= presenter->GetNextDrawable();
	VkCommandBufferBeginInfo	begininfo		= {};
	VkRenderPassBeginInfo		passbegininfo	= {};
	VkClearValue				clearcolors[2];
	VkCommandBuffer				primarycmdbuff	= presenter->GetCommandBuffer(currentdrawable);
	VkImage						swapchainimg	= presenter->GetSwapchainImage(currentdrawable);
	VkImage						depthbuffer		= presenter->GetDepthBuffer()->GetImage();
	VkResult					res;

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
		// TODO: render

		// render help text
		screenquad->GetPipeline()->SetViewport(10.0f, 10.0f, 512.0f, 512.0f);
		screenquad->GetPipeline()->SetScissor(10, 10, 512, 512);

		screenquad->Draw(primarycmdbuff);
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
