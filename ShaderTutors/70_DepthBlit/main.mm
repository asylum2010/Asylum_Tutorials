
#import <MetalKit/MetalKit.h>
#import <iostream>

#include "application.h"
#include "mtlext.h"
#include "basiccamera.h"

#define NUM_QUEUED_FRAMES	3

// helper macros
#define TITLE				"Shader sample 70: Metal depth blit test"

// sample structures
struct CombinedUniformData
{
	// byte offset 0
	struct BlinnphongUniformData {
		Math::Matrix world;
		Math::Matrix viewproj;
		Math::Vector4 lightpos;
		Math::Vector4 eyepos;
	} blinnphonguniforms;	// 160 B
	
	Math::Vector4 padding1[6];	// 96 B
	
	// byte offset 256
	struct ColordGridUniformData {
		Math::Vector4 time;
	} colorgriduniforms;	// 16 B
	
	Math::Vector4 padding2[15];	// 240 B
};

// sample variables
Application*				app					= nullptr;
MTKView*					mtkview				= nil;

id<MTLDevice>				device				= nil;
id<MTLCommandQueue>			commandqueue		= nil;
id<MTLLibrary>				defaultlibrary		= nil;
id<MTLComputePipelineState>	computestate		= nil;
id<MTLRenderPipelineState> 	renderstate			= nil;
id<MTLDepthStencilState>	depthstate			= nil;
id<MTLBuffer>				uniformbuffer		= nil;
id<MTLTexture>				texture				= nil;
id<MTLTexture>				helptext			= nil;

id<MTLTexture>				bottomlayer			= nil;
id<MTLTexture>				bottomlayerdepth	= nil;
id<MTLTexture>				feedbacklayer		= nil;
id<MTLTexture>				feedbacklayerdepth	= nil;

id<MTLRenderPipelineState>	combinestate		= nil;
id<MTLDepthStencilState>	ztestoff			= nil;

BasicCamera					camera;
MetalMesh*					mesh				= nullptr;
MetalScreenQuad*			screenquad			= nullptr;
dispatch_semaphore_t		flightsema			= 0;
uint64_t 					currentframe		= 0;
bool						useworkaround		= false;

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
	NSString* path = [[NSBundle mainBundle] pathForResource:@"teapot" ofType:@"qm"];
	mesh = MetalMesh::LoadFromQM(device, commandqueue, [path UTF8String]);

	if (mesh == nullptr) {
		NSLog(@"Error: Could not load mesh!");
		return false;
	}
	
	// upload to VRAM
	MetalTemporaryCommandBuffer(commandqueue, true, [&](id<MTLCommandBuffer> transfercmd) -> bool {
		mesh->UploadToVRAM(transfercmd);
		return true;
	});
	
	mesh->DeleteStagingBuffers();
	
	// create empty texture
	MTLTextureDescriptor* texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
	
	texdesc.storageMode = MTLStorageModePrivate;
	texdesc.usage = MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;
	
	texture = [device newTextureWithDescriptor:texdesc];
	
	// create render targets
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB width:screenwidth height:screenheight mipmapped:NO];
	
	texdesc.storageMode = MTLStorageModePrivate;
	texdesc.usage = MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
	
	bottomlayer = [device newTextureWithDescriptor:texdesc];
	feedbacklayer = [device newTextureWithDescriptor:texdesc];
	
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:screenwidth height:screenheight mipmapped:NO];
	
	texdesc.storageMode = MTLStorageModePrivate;
	texdesc.usage = MTLTextureUsageRenderTarget;
	
	bottomlayerdepth = [device newTextureWithDescriptor:texdesc];
	feedbacklayerdepth = [device newTextureWithDescriptor:texdesc];
	
	// create uniform buffer (NOTE: 2 teapots)
	uniformbuffer = [device newBufferWithLength:2 * NUM_QUEUED_FRAMES * sizeof(CombinedUniformData) options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	
	// create pipelines
	MTLRenderPipelineDescriptor* pipelinedesc = [[MTLRenderPipelineDescriptor alloc] init];
	NSError* error = nil;
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_blinnphong"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_blinnphong"];
	pipelinedesc.colorAttachments[0].pixelFormat = mtkview.colorPixelFormat;
	pipelinedesc.depthAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.stencilAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.vertexDescriptor = mesh->GetVertexLayout();
	
	renderstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!renderstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_screenquad"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_screenquad"];
	
	pipelinedesc.colorAttachments[0].pixelFormat = bottomlayer.pixelFormat;
	pipelinedesc.colorAttachments[0].blendingEnabled = YES;
	pipelinedesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelinedesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	
	pipelinedesc.depthAttachmentPixelFormat = bottomlayerdepth.pixelFormat;
	pipelinedesc.stencilAttachmentPixelFormat = bottomlayerdepth.pixelFormat;
	pipelinedesc.vertexDescriptor = nil;

	combinestate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!combinestate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	computestate = [device newComputePipelineStateWithFunction:[defaultlibrary newFunctionWithName:@"coloredgrid"] error:&error];
	
	if (!computestate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	// create depth-stencil states
	MTLDepthStencilDescriptor* depthdesc = [[MTLDepthStencilDescriptor alloc] init];
	
	depthdesc.depthCompareFunction = MTLCompareFunctionLess;
	depthdesc.depthWriteEnabled = YES;
	
	depthstate = [device newDepthStencilStateWithDescriptor:depthdesc];
	
	depthdesc.depthWriteEnabled = NO;
	depthdesc.depthCompareFunction = MTLCompareFunctionAlways;

	ztestoff = [device newDepthStencilStateWithDescriptor:depthdesc];

	// render text
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:512 height:512 mipmapped:NO];
	texdesc.usage = MTLTextureUsageShaderRead;

	helptext = [device newTextureWithDescriptor:texdesc];
	
	MetalRenderText("Press Space to toggle MTLFence", helptext);
	
	screenquad = new MetalScreenQuad(device, defaultlibrary, mtkview.colorPixelFormat, helptext);
	
	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetDistance(3);
	camera.SetClipPlanes(0.1f, 20.0f);
	
	return true;
}

void UninitScene()
{
	delete mesh;
	delete screenquad;
}

void KeyUp(KeyCode key)
{
	switch (key) {
		case KeyCodeSpace:
			useworkaround = !useworkaround;
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
	
	Math::Matrix world, view, proj;
	Math::Matrix viewproj, tmp;
	Math::Vector3 eye;
	
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
	camera.GetEyePosition(eye);
	
	MTLRenderPassDescriptor* renderpassdesc = mtkview.currentRenderPassDescriptor;
	CombinedUniformData* uniforms = ((CombinedUniformData*)[uniformbuffer contents] + currentframe);
	
	// setup uniforms
	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixIdentity(world);

	world._41 = -0.108f;
	world._42 = -0.7875f;
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 1, 0, 0);
	Math::MatrixMultiply(world, world, tmp);
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 0, 1, 0);
	Math::MatrixMultiply(world, world, tmp);
	
	if (renderpassdesc) {
		id<MTLComputeCommandEncoder> computeencoder = [commandbuffer computeCommandEncoder];
		
		uniforms->colorgriduniforms.time.x = time;
		
		[computeencoder setTexture:texture atIndex:0];
		[computeencoder setBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, colorgriduniforms)) atIndex:0];
		[computeencoder setComputePipelineState:computestate];
		[computeencoder dispatchThreadgroups:MTLSizeMake(8, 8, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
		[computeencoder endEncoding];
		
		// save these
		id<MTLTexture> oldcolor = renderpassdesc.colorAttachments[0].texture;
		id<MTLTexture> olddepth = renderpassdesc.depthAttachment.texture;
		id<MTLTexture> oldstencil = renderpassdesc.stencilAttachment.texture;
		
		// render first teapot (uniform offsets: 0, 1, 2)
		world._41 -= 0.75f;
		
		uniforms->blinnphonguniforms.world = world;
		uniforms->blinnphonguniforms.viewproj = viewproj;
		uniforms->blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
		uniforms->blinnphonguniforms.lightpos = Math::Vector4(6, 3, 10, 1);
		
		renderpassdesc.colorAttachments[0].texture = bottomlayer;
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
		renderpassdesc.colorAttachments[0].storeAction = MTLStoreActionStore;
		renderpassdesc.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
		
		renderpassdesc.depthAttachment.texture = bottomlayerdepth;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionStore;
		
		renderpassdesc.stencilAttachment.texture = bottomlayerdepth;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.stencilAttachment.storeAction = MTLStoreActionStore;
		
		id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			[encoder setRenderPipelineState:renderstate];
			[encoder setDepthStencilState:depthstate];
			
			[encoder setVertexBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData)) atIndex:1];
			[encoder setFragmentTexture:texture atIndex:0];
			
			mesh->Draw(encoder);
		}
		[encoder endEncoding];
		
		// blit depth to feedback layer
		id<MTLFence> dependency = [device newFence];	// NOTE: Intel bug
		
		id<MTLBlitCommandEncoder> blitencoder = [commandbuffer blitCommandEncoder];
		{
			[blitencoder copyFromTexture:bottomlayerdepth
							 sourceSlice:0
							 sourceLevel:0
							 sourceOrigin:MTLOriginMake(0, 0, 0)
							  sourceSize:MTLSizeMake(bottomlayerdepth.width, bottomlayerdepth.height, 1)
							   toTexture:feedbacklayerdepth
						destinationSlice:0
						destinationLevel:0
					   destinationOrigin:MTLOriginMake(0, 0, 0)];
			
			if (useworkaround)
				[blitencoder updateFence:dependency];
		}
		[blitencoder endEncoding];
		
		// render second teapot (uniform offsets: 3, 4, 5)
		uniforms = ((CombinedUniformData*)[uniformbuffer contents] + NUM_QUEUED_FRAMES + currentframe);
		
		world._41 += 1.5f;
		
		uniforms->blinnphonguniforms.world = world;
		uniforms->blinnphonguniforms.viewproj = viewproj;
		uniforms->blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
		uniforms->blinnphonguniforms.lightpos = Math::Vector4(6, 3, 10, 1);
		
		// NOTE: load the blitted depth texture
		renderpassdesc.colorAttachments[0].texture = feedbacklayer;
		
		renderpassdesc.depthAttachment.texture = feedbacklayerdepth;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionDontCare;
		
		renderpassdesc.stencilAttachment.texture = feedbacklayerdepth;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.stencilAttachment.storeAction = MTLStoreActionDontCare;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			if (useworkaround)
				[encoder waitForFence:dependency beforeStages:MTLRenderStageFragment];
			
			[encoder setCullMode:MTLCullModeFront];
			[encoder setRenderPipelineState:renderstate];
			[encoder setDepthStencilState:depthstate];
			
			[encoder setVertexBuffer:uniformbuffer offset:((NUM_QUEUED_FRAMES + currentframe) * sizeof(CombinedUniformData)) atIndex:1];
			[encoder setFragmentTexture:texture atIndex:0];
			
			mesh->Draw(encoder);
		}
		[encoder endEncoding];
		
		// combine
		renderpassdesc.colorAttachments[0].texture = oldcolor;
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
		renderpassdesc.colorAttachments[0].storeAction = MTLStoreActionStore;
		renderpassdesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0f, 0.0103f, 0.0707f, 1.0f);
		
		renderpassdesc.depthAttachment.texture = olddepth;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionDontCare;
		
		renderpassdesc.stencilAttachment.texture = oldstencil;
		renderpassdesc.stencilAttachment.loadAction = MTLLoadActionClear;
		renderpassdesc.stencilAttachment.storeAction = MTLStoreActionDontCare;

		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeBack];
			[encoder setRenderPipelineState:combinestate];
			[encoder setDepthStencilState:ztestoff];
			
			[encoder setFragmentTexture:bottomlayer atIndex:0];
			[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
			
			[encoder setFragmentTexture:feedbacklayer atIndex:0];
			[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		}
		[encoder endEncoding];
		
		// render text
		MTLViewport viewport;
		auto depthattachment = renderpassdesc.depthAttachment;
		auto stencilattachment = renderpassdesc.stencilAttachment;
		
		renderpassdesc.depthAttachment = nil;
		renderpassdesc.stencilAttachment = nil;
		
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
		
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
	time += elapsedtime;
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
	
	app->Run();
	delete app;
	
	return 0;
}
