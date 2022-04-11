
#import <MetalKit/MetalKit.h>
#import <iostream>

#include "application.h"
#include "mtlext.h"
#include "basiccamera.h"

#define NUM_QUEUED_FRAMES	3
#define GRID_SIZE			10			// N^3 objects
#define UNIFORM_COPIES		512			// fixed number of uniform slots
#define OBJECT_SCALE		0.35f

// helper macros
#define TITLE				"Shader sample 70: Synchronizing uniform data mid-frame"

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
	struct ColordGridUniformData {
		Math::Vector4 time;
	} colorgriduniforms;	// 16 B
	
	Math::Vector4 padding2[15];	// 240 B
};

// sample variables
Application*				app				= nullptr;
MTKView*					mtkview			= nil;

id<MTLDevice>				device			= nil;
id<MTLCommandQueue>			commandqueue	= nil;
id<MTLLibrary>				defaultlibrary	= nil;
id<MTLComputePipelineState>	computestate	= nil;
id<MTLRenderPipelineState> 	renderstate		= nil;
id<MTLDepthStencilState>	depthstate		= nil;
id<MTLBuffer>				uniformbuffer	= nil;		// ringbuffer
id<MTLTexture>				texture			= nil;
id<MTLTexture>				helptext		= nil;

BasicCamera					camera;
MetalScreenQuad*			screenquad		= nullptr;
MetalMesh*					mesh			= nullptr;
dispatch_semaphore_t		flightsema		= 0;
uint64_t 					currentframe	= 0;
uint64_t					currentcopy		= 0;		// current block in the ringbuffer
bool						useorphaning	= false;

Math::Matrix				viewproj;
Math::Vector3				eye;

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
	
	// create uniform buffer
	uniformbuffer = [device newBufferWithLength:UNIFORM_COPIES * NUM_QUEUED_FRAMES * sizeof(CombinedUniformData) options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	
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
	
	computestate = [device newComputePipelineStateWithFunction:[defaultlibrary newFunctionWithName:@"coloredgrid"] error:&error];
	
	if (!computestate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	// create depth-stencil state
	MTLDepthStencilDescriptor* depthdesc = [[MTLDepthStencilDescriptor alloc] init];
	
	depthdesc.depthCompareFunction = MTLCompareFunctionLess;
	depthdesc.depthWriteEnabled = YES;
	
	depthstate = [device newDepthStencilStateWithDescriptor:depthdesc];

	// render text
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:512 height:512 mipmapped:NO];
	texdesc.usage = MTLTextureUsageShaderRead;
	
	helptext = [device newTextureWithDescriptor:texdesc];
	
	MetalRenderText("Mouse left - Orbit camera\n\n1 - Synchronize with commit-wait\n2 - Synchronize with orphaning", helptext);
	
	screenquad = new MetalScreenQuad(device, defaultlibrary, mtkview.colorPixelFormat, helptext);
	
	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(80));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(10.0f);
	
	return true;
}

void UninitScene()
{
	delete screenquad;
}

void KeyUp(KeyCode key)
{
	switch (key) {
		case KeyCode1:
			useorphaning = false;
			break;
			
		case KeyCode2:
			useorphaning = true;
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
	}
}

void Update(float delta)
{
	camera.Update(delta);
}

void SynchronizeWithWait(id<MTLCommandBuffer>* commandbuffer, MTLRenderPassDescriptor* renderpassdesc, const Math::Matrix& world, float time)
{
	if (currentcopy >= UNIFORM_COPIES) {
		// scenario #1: finish current command buffer
		[*commandbuffer commit];
		[*commandbuffer waitUntilCompleted];
		
		*commandbuffer = [commandqueue commandBuffer];
		currentcopy = 0;
	}
	
	CombinedUniformData* uniforms = ((CombinedUniformData*)[uniformbuffer contents] + currentframe * UNIFORM_COPIES + currentcopy);
	
	uniforms->colorgriduniforms.time.x = 2 * time;
	
	if (currentcopy == 0) {
		id<MTLComputeCommandEncoder> computeencoder = [*commandbuffer computeCommandEncoder];
		
		[computeencoder setTexture:texture atIndex:0];
		[computeencoder setBuffer:uniformbuffer offset:(currentframe * UNIFORM_COPIES * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, colorgriduniforms)) atIndex:0];
		[computeencoder setComputePipelineState:computestate];
		[computeencoder dispatchThreadgroups:MTLSizeMake(8, 8, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
		[computeencoder endEncoding];
	}
	
	Math::Matrix worldinv;
	Math::MatrixInverse(worldinv, world);
	
	uniforms->blinnphonguniforms.world = world;
	uniforms->blinnphonguniforms.worldinv = worldinv;
	uniforms->blinnphonguniforms.viewproj = viewproj;
	uniforms->blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
	uniforms->blinnphonguniforms.lightpos = Math::Vector4(6, 3, 10, 1);
	uniforms->blinnphonguniforms.uvscale = Math::Vector4(1, 1, 0, 0);
	
	id<MTLRenderCommandEncoder> encoder = [*commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
	{
		[encoder setCullMode:MTLCullModeFront];
		[encoder setRenderPipelineState:renderstate];
		[encoder setDepthStencilState:depthstate];
		
		[encoder setVertexBuffer:uniformbuffer offset:((currentframe * UNIFORM_COPIES + currentcopy) * sizeof(CombinedUniformData)) atIndex:1];
		[encoder setFragmentTexture:texture atIndex:0];
		
		mesh->Draw(encoder);
	}
	[encoder endEncoding];
}

void SynchronizeWithOrphaning(id<MTLCommandBuffer> commandbuffer, id<MTLRenderCommandEncoder> encoder, const Math::Matrix& world, float time)
{
	if (currentcopy >= UNIFORM_COPIES) {
		// scenario #2: just orphan the damn thing
		uniformbuffer = [device newBufferWithLength:UNIFORM_COPIES * NUM_QUEUED_FRAMES * sizeof(CombinedUniformData) options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
		currentcopy = 0;
	}
	
	CombinedUniformData* uniforms = ((CombinedUniformData*)[uniformbuffer contents] + currentframe * UNIFORM_COPIES + currentcopy);
	
	Math::Matrix worldinv;
	Math::MatrixInverse(worldinv, world);
	
	uniforms->blinnphonguniforms.world = world;
	uniforms->blinnphonguniforms.worldinv = worldinv;
	uniforms->blinnphonguniforms.viewproj = viewproj;
	uniforms->blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
	uniforms->blinnphonguniforms.lightpos = Math::Vector4(6, 3, 10, 1);
	uniforms->blinnphonguniforms.uvscale = Math::Vector4(1, 1, 0, 0);
	
	[encoder setVertexBuffer:uniformbuffer offset:((currentframe * UNIFORM_COPIES + currentcopy) * sizeof(CombinedUniformData)) atIndex:1];
	[encoder setFragmentTexture:texture atIndex:0];
	
	mesh->Draw(encoder);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;
	
	Math::Matrix world, view, proj;
	
	// will wait until a command buffer finished execution
	dispatch_semaphore_wait(flightsema, DISPATCH_TIME_FOREVER);
	
	__block dispatch_semaphore_t blocksema = flightsema;
	id<MTLCommandBuffer> commandbuffer = [commandqueue commandBuffer];
	
	camera.Animate(alpha);
	
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);
	
	Math::MatrixMultiply(viewproj, view, proj);
	
	MTLRenderPassDescriptor* renderpassdesc = mtkview.currentRenderPassDescriptor;

	if (renderpassdesc) {
		currentcopy = 0;
		
		for (int i = 0; i < GRID_SIZE; ++i) {
			for (int j = 0; j < GRID_SIZE; ++j) {
				for (int k = 0; k < GRID_SIZE; ++k) {
					Math::MatrixScaling(world, OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE);
					
					world._41 = GRID_SIZE * -0.5f + i;
					world._42 = GRID_SIZE * -0.5f + j;
					world._43 = GRID_SIZE * -0.5f + k;
					
					// NOTE: switching between these is dangerous, but hey...
					if (useorphaning) {
						// NOTE: scenario #2
						if (currentcopy == 0) {
						//if (currentcopy < UNIFORM_COPIES) {	// PERFORMANCE PENALTY
							CombinedUniformData* uniforms = ((CombinedUniformData*)[uniformbuffer contents] + currentframe * UNIFORM_COPIES + currentcopy);
							uniforms->colorgriduniforms.time.x = 8 * time + currentcopy * elapsedtime + Math::HALF_PI;
							
							id<MTLComputeCommandEncoder> computeencoder = [commandbuffer computeCommandEncoder];
							
							[computeencoder setTexture:texture atIndex:0];
							[computeencoder setBuffer:uniformbuffer offset:((currentframe * UNIFORM_COPIES + currentcopy) * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, colorgriduniforms)) atIndex:0];
							[computeencoder setComputePipelineState:computestate];
							[computeencoder dispatchThreadgroups:MTLSizeMake(8, 8, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
							[computeencoder endEncoding];
						} // else it will use the last one; no big deal...
						
						id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
						{
							[encoder setCullMode:MTLCullModeFront];
							[encoder setRenderPipelineState:renderstate];
							[encoder setDepthStencilState:depthstate];
							
							SynchronizeWithOrphaning(commandbuffer, encoder, world, time);
						}
						[encoder endEncoding];
					} else {
						// NOTE: scenario #1
						SynchronizeWithWait(&commandbuffer, renderpassdesc, world, time);
					}
					
					// only first encoder clears
					renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
					renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
					renderpassdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
					
					++currentcopy;
				}
			}
		}

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
		
		id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
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

	// last command buffer terminates frame
	[commandbuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
		dispatch_semaphore_signal(blocksema);
	}];

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
