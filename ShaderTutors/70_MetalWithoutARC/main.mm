
#import <MetalKit/MetalKit.h>
#import <iostream>

#include "application.h"
#include "mtlext.h"
#include "basiccamera.h"

#define NUM_QUEUED_FRAMES	3

// helper macros
#define TITLE				"Shader sample 70: Metal w/o ARC"

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
Application*				app				= nullptr;
MTKView*					mtkview			= nil;

id<MTLDevice>				device			= nil;
id<MTLCommandQueue>			commandqueue	= nil;
id<MTLLibrary>				defaultlibrary	= nil;
id<MTLComputePipelineState>	computestate	= nil;
id<MTLRenderPipelineState> 	renderstate		= nil;
id<MTLDepthStencilState>	depthstate		= nil;
id<MTLTexture>				texture			= nil;

BasicCamera					camera;
MetalMesh*					mesh			= nullptr;
dispatch_semaphore_t		flightsema		= 0;
uint64_t 					currentframe	= 0;

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

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetDistance(3);
	camera.SetClipPlanes(0.1f, 20.0f);
	
	return true;
}

void UninitScene()
{
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
	
	// create uniform buffer (want to test how it leaks)
	id<MTLBuffer> uniformbuffer = [device newBufferWithLength:sizeof(CombinedUniformData) options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	
	MTLRenderPassDescriptor* renderpassdesc = mtkview.currentRenderPassDescriptor;
	CombinedUniformData* uniforms = (CombinedUniformData*)[uniformbuffer contents];
	
	// setup uniforms
	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixIdentity(world);

	world._41 = -0.108f;
	world._42 = -0.7875f;
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 1, 0, 0);
	Math::MatrixMultiply(world, world, tmp);
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 0, 1, 0);
	Math::MatrixMultiply(world, world, tmp);

	uniforms->blinnphonguniforms.world = world;
	uniforms->blinnphonguniforms.viewproj = viewproj;
	uniforms->blinnphonguniforms.eyepos = Math::Vector4(eye, 1);
	uniforms->blinnphonguniforms.lightpos = Math::Vector4(6, 3, 10, 1);
	
	uniforms->colorgriduniforms.time.x = time;
	time += elapsedtime;
	
	if (renderpassdesc) {
		id<MTLComputeCommandEncoder> computeencoder = [commandbuffer computeCommandEncoder];
		
		[computeencoder setTexture:texture atIndex:0];
		[computeencoder setBuffer:uniformbuffer offset:offsetof(CombinedUniformData, colorgriduniforms) atIndex:0];
		[computeencoder setComputePipelineState:computestate];
		[computeencoder dispatchThreadgroups:MTLSizeMake(8, 8, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
		[computeencoder endEncoding];
		
		id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			[encoder setRenderPipelineState:renderstate];
			[encoder setDepthStencilState:depthstate];
			
			[encoder setVertexBuffer:uniformbuffer offset:0 atIndex:1];
			[encoder setFragmentTexture:texture atIndex:0];
			
			mesh->Draw(encoder);
		}
		[encoder endEncoding];
		
		[commandbuffer presentDrawable:mtkview.currentDrawable];
		currentframe = (currentframe + 1) % NUM_QUEUED_FRAMES;
	}
	
	// even to we are in an @autoreleasepool block, have to release Metal objects
	[uniformbuffer release];

	[commandbuffer commit];
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
