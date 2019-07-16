
#import <MetalKit/MetalKit.h>
#import <iostream>

#include "application.h"
#include "mtlext.h"
#include "spectatorcamera.h"

#define NUM_QUEUED_FRAMES	3
#define METERS_PER_UNIT		0.01f
#define NUM_LIGHTS			512
#define LIGHT_RADIUS		2.0f

// helper macros
#define TITLE				"Shader sample 70: Forward+ rendering"

// sample structures
struct CombinedUniformData
{
	// byte offset 0
	struct ZOnlyUniformData {
		Math::Matrix world;
		Math::Matrix viewproj;
	} zonlyuniforms;	// 128 B
	
	Math::Vector4 padding1[8];	// 128 B
	
	// byte offset 256
	struct LightCullUniformData {
		Math::Matrix view;
		Math::Matrix proj;
		Math::Matrix viewproj;
		Math::Vector4 clipplanes;	// z = interpolation value
	} lightculluniforms;	// 208 B
	
	Math::Vector4 padding2[3];	// 48 B
	
	// byte offset 512
	struct AccumVertexUniformData {
		Math::Matrix world;
		Math::Matrix viewproj;
		Math::Vector4 eyepos;
	} accumvertuniforms;	// 144 B
	
	Math::Vector4 padding3[7];	// 112 B
	
	// byte offset 768
	struct AccumFragmentUniformData {
		uint32_t numtilesx;
		uint32_t numtilesy;
		float alpha;
		float unused;
	} accumfraguniforms;	// 16 B
	
	Math::Vector4 padding4[15];	// 240 B
	
	// byte offset 1024
	struct FlaresPassUniformData {
		Math::Matrix view;
		Math::Matrix proj;
		Math::Vector4 params;
	} flaresuniforms; // 144 B
	
	Math::Vector4 padding5[7];	// 112 B
};

static_assert(sizeof(CombinedUniformData::AccumFragmentUniformData) == 16, "sizeof(AccumFragmentUniformData) must be 16 bytes");

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
MTKView*					mtkview				= nil;

id<MTLDevice>				device				= nil;
id<MTLCommandQueue>			commandqueue		= nil;
id<MTLLibrary>				defaultlibrary		= nil;

id<MTLComputePipelineState>	computestate		= nil;
id<MTLRenderPipelineState>	zonlystate 			= nil;
id<MTLRenderPipelineState> 	forwardstate		= nil;
id<MTLRenderPipelineState>	tonemapstate		= nil;
id<MTLRenderPipelineState>	flaresstate			= nil;
id<MTLDepthStencilState>	zwriteon			= nil;
id<MTLDepthStencilState>	zwriteoff			= nil;
id<MTLDepthStencilState>	ztestoff			= nil;

id<MTLTexture>				rendertarget		= nil;
id<MTLTexture>				ambientcube			= nil;
id<MTLTexture>				flaretexture		= nil;
id<MTLTexture>				supplynormalmap		= nil;
id<MTLTexture>				helptext			= nil;

id<MTLBuffer>				uniformbuffer		= nil;
id<MTLBuffer>				headbuffer			= nil;		// head of linked lists
id<MTLBuffer>				nodebuffer			= nil;		// nodes of linked lists
id<MTLBuffer>				lightbuffer			= nil;		// light particles
id<MTLBuffer>				counterbuffer		= nil;		// atomic counter

Math::AABox					particlespace(-13.2f, 2.0f, -5.7f, 13.2f, 14.4f, 5.7f);

SpectatorCamera				camera;
MetalScreenQuad*			screenquad			= nullptr;
MetalMesh*					model				= nullptr;
dispatch_semaphore_t		flightsema			= 0;
uint64_t 					currentframe		= 0;
uint64_t					currentphysicsframe	= 0;

uint64_t					headbuffersize		= 0;
uint64_t					nodebuffersize		= 0;
uint64_t					lightbuffersize		= 0;

uint32_t					workgroupsx			= 0;
uint32_t					workgroupsy			= 0;

bool						drawtext			= true;
bool						freezetime			= false;

void GenerateParticles();
void UpdateParticles(float dt);

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

	// load model
	NSString* path = [[NSBundle mainBundle] pathForResource:@"sponza" ofType:@"qm"];
	model = MetalMesh::LoadFromQM(device, commandqueue, [path UTF8String]);

	if (model == nullptr) {
		NSLog(@"Error: Could not load mesh!");
		return false;
	}
	
	model->GenerateTangentFrame(device);
	model->EnableSubset(4, false);
	
	// upload to VRAM
	MetalTemporaryCommandBuffer(commandqueue, true, [&](id<MTLCommandBuffer> transfercmd) -> bool {
		model->UploadToVRAM(transfercmd);
		return true;
	});
	
	model->DeleteStagingBuffers();
	
	path = [[NSBundle mainBundle] pathForResource:@"flare1" ofType:@"png"];
	flaretexture = MetalCreateTextureFromFile(device, commandqueue, [path UTF8String], true);
	
	// create substitute texture
	MTLTextureDescriptor* texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:1 height:1 mipmapped:NO];
	MTLRegion region = { 0, 0, 0, 1, 1, 1 };
	
	supplynormalmap = [device newTextureWithDescriptor:texdesc];
	
	uint32_t data = 0xff7f7fff;	// (0.5f, 0.5f, 1)
	[supplynormalmap replaceRegion:region mipmapLevel:0 withBytes:&data bytesPerRow:4];
	
	for (uint32_t i = 0; i < model->GetNumSubsets(); ++i) {
		MetalMaterial& material = model->GetMaterial(i);
		
		if (material.normalMap == nil)
			material.normalMap = supplynormalmap;
	}
	
	// load ambient cubemap
	path = [[NSBundle mainBundle] pathForResource:@"uffizi_diff_irrad" ofType:@"dds"];
	ambientcube = MetalCreateTextureFromDDS(device, commandqueue, [path UTF8String], false);
	
	// generate particles
	GenerateParticles();

	// create buffers
	workgroupsx = (screenwidth + (screenwidth % 16)) / 16;
	workgroupsy = (screenheight + (screenheight % 16)) / 16;
	
	size_t numtiles = workgroupsx * workgroupsy;
	size_t headsize = 16;	// start, count, pad, pad
	size_t nodesize = 16;	// light index, next, pad, pad
	
	headbuffersize = numtiles * headsize;
	nodebuffersize = numtiles * nodesize * 1024;	// 4 MB should be enough
	lightbuffersize = NUM_LIGHTS * sizeof(LightParticle);
	
	headbuffer = [device newBufferWithLength:NUM_QUEUED_FRAMES * headbuffersize options:MTLResourceStorageModePrivate];
	nodebuffer = [device newBufferWithLength:NUM_QUEUED_FRAMES * nodebuffersize options:MTLResourceStorageModePrivate];
	counterbuffer = [device newBufferWithLength:NUM_QUEUED_FRAMES * 4 options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	uniformbuffer = [device newBufferWithLength:NUM_QUEUED_FRAMES * sizeof(CombinedUniformData) options:MTLStorageModeShared|MTLResourceCPUCacheModeWriteCombined];
	
	// create render target
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float width:screenwidth height:screenheight mipmapped:NO];
	
	texdesc.storageMode = MTLStorageModePrivate;
	texdesc.usage = MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
	
	rendertarget = [device newTextureWithDescriptor:texdesc];
	
	// create pipelines
	MTLRenderPipelineDescriptor* pipelinedesc = [[MTLRenderPipelineDescriptor alloc] init];
	NSError* error = nil;

	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_zonly"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_zonly"];
	
	pipelinedesc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
	pipelinedesc.depthAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.stencilAttachmentPixelFormat = mtkview.depthStencilPixelFormat;
	pipelinedesc.vertexDescriptor = model->GetVertexLayout(pipelinedesc.vertexFunction.vertexAttributes);
	
	zonlystate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!zonlystate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_lightaccum"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_lightaccum"];

	pipelinedesc.colorAttachments[0].blendingEnabled = YES;
	pipelinedesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	pipelinedesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
	pipelinedesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
	pipelinedesc.vertexDescriptor = model->GetVertexLayout();
	
	forwardstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!forwardstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_tonemap"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_tonemap"];
	
	pipelinedesc.colorAttachments[0].pixelFormat = mtkview.colorPixelFormat;
	pipelinedesc.colorAttachments[0].blendingEnabled = NO;
	pipelinedesc.vertexDescriptor = nil;
	
	tonemapstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!tonemapstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	pipelinedesc.vertexFunction = [defaultlibrary newFunctionWithName:@"vs_flares"];
	pipelinedesc.fragmentFunction = [defaultlibrary newFunctionWithName:@"ps_flares"];
	
	pipelinedesc.colorAttachments[0].blendingEnabled = YES;
	
	flaresstate = [device newRenderPipelineStateWithDescriptor:pipelinedesc error:&error];
	
	if (!flaresstate) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	// create compute pipeline
	MTLFunctionConstantValues* cullconstants = [[MTLFunctionConstantValues alloc] init];
	uint32_t numlights = NUM_LIGHTS;
	
	[cullconstants setConstantValue:&numlights type:MTLDataTypeUInt atIndex:0];

	id<MTLFunction> cullfunc = [defaultlibrary newFunctionWithName:@"lightcull" constantValues:cullconstants error:&error];
	
	if (!cullfunc) {
		NSLog(@"Error: %@", error);
		return false;
	}
	
	computestate = [device newComputePipelineStateWithFunction:cullfunc error:&error];

	if (!computestate) {
		NSLog(@"Error: %@", error);
		return false;
	}

	// create depth-stencil state
	MTLDepthStencilDescriptor* depthdesc = [[MTLDepthStencilDescriptor alloc] init];
	
	depthdesc.depthCompareFunction = MTLCompareFunctionLess;
	depthdesc.depthWriteEnabled = YES;
	
	zwriteon = [device newDepthStencilStateWithDescriptor:depthdesc];

	depthdesc.depthCompareFunction = MTLCompareFunctionLessEqual;
	depthdesc.depthWriteEnabled = NO;
	
	zwriteoff = [device newDepthStencilStateWithDescriptor:depthdesc];
	
	depthdesc.depthCompareFunction = MTLCompareFunctionAlways;
	depthdesc.depthWriteEnabled = NO;
	
	ztestoff = [device newDepthStencilStateWithDescriptor:depthdesc];

	// render text
	texdesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:512 height:512 mipmapped:NO];
	texdesc.usage = MTLTextureUsageShaderRead;

	helptext = [device newTextureWithDescriptor:texdesc];
	
	MetalRenderText("Use WASD and mouse to move around\n\nT - Freeze time\nH - Toggle help text", helptext);
	
	screenquad = new MetalScreenQuad(device, defaultlibrary, mtkview.colorPixelFormat, helptext);
	
	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetEyePosition(-0.12f, 3.0f, 0);
	camera.SetOrientation(-Math::HALF_PI, 0, 0);
	
	return true;
}

void UninitScene()
{
	delete model;
	delete screenquad;
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
	
	lightbuffer = [device newBufferWithLength:NUM_QUEUED_FRAMES * NUM_LIGHTS * sizeof(LightParticle) options:MTLCPUCacheModeDefaultCache|MTLStorageModeShared];
	MTL_ASSERT(lightbuffer != nil);
	
	LightParticle* particles = (LightParticle*)[lightbuffer contents];
	
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
		
		for (int j = 0; j < NUM_QUEUED_FRAMES; ++j) {
			particles[i + j * NUM_LIGHTS].color	= colors[c];
			particles[i + j * NUM_LIGHTS].radius = LIGHT_RADIUS;
			
			particles[i + j * NUM_LIGHTS].previous = Math::Vector4(x, y, z, 1);
			particles[i + j * NUM_LIGHTS].current = Math::Vector4(x, y, z, 1);
			particles[i + j * NUM_LIGHTS].velocity = Math::Vector3(0, v * d, 0);
		}
	}
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
	
	uint32_t prevphysicsframe = (currentphysicsframe + NUM_QUEUED_FRAMES - 1) % NUM_QUEUED_FRAMES;
	
	LightParticle* particles = (LightParticle*)[lightbuffer contents];
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
	
	currentphysicsframe = (currentphysicsframe + 1) % NUM_QUEUED_FRAMES;
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
	
	Math::Matrix world, view, proj;
	Math::Matrix viewproj;
	Math::Vector3 eye;
	
	if (freezetime) {
		if (stopalpha == -1)
			stopalpha = alpha;
	} else {
		stopalpha = -1;
	}
	
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
	float interpolator = (freezetime ? stopalpha : alpha);
	
	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixScaling(world, METERS_PER_UNIT, METERS_PER_UNIT, METERS_PER_UNIT);
	
	world._41 = 60.518921f * METERS_PER_UNIT;
	world._42 = (778.0f - 651.495361f) * METERS_PER_UNIT;
	world._43 = -38.690552f * METERS_PER_UNIT;

	uniforms->zonlyuniforms.world = world;
	uniforms->zonlyuniforms.viewproj = viewproj;
	
	uniforms->lightculluniforms.view = view;
	uniforms->lightculluniforms.proj = proj;
	uniforms->lightculluniforms.viewproj = viewproj;
	
	uniforms->lightculluniforms.clipplanes.x = camera.GetNearPlane();
	uniforms->lightculluniforms.clipplanes.y = camera.GetFarPlane();
	uniforms->lightculluniforms.clipplanes.z = interpolator;
	
	uniforms->accumvertuniforms.world = world;
	uniforms->accumvertuniforms.viewproj = viewproj;
	uniforms->accumvertuniforms.eyepos = Math::Vector4(eye, 1);
	
	uniforms->accumfraguniforms.numtilesx = workgroupsx;
	uniforms->accumfraguniforms.numtilesy = workgroupsy;
	uniforms->accumfraguniforms.alpha = interpolator;
	
	uniforms->flaresuniforms.view = view;
	uniforms->flaresuniforms.proj = proj;
	uniforms->flaresuniforms.params = Math::Vector4(interpolator, time, 0, 0);
	
	if (!freezetime)
		time += elapsedtime;
	
	if (renderpassdesc) {
		id<MTLTexture> drawable = renderpassdesc.colorAttachments[0].texture;
		
		// STEP 1: z-only pass
		renderpassdesc.colorAttachments[0].texture = rendertarget;
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionStore;
		
		id<MTLRenderCommandEncoder> encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			[encoder setRenderPipelineState:zonlystate];
			[encoder setDepthStencilState:zwriteon];
			
			[encoder setVertexBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, zonlyuniforms)) atIndex:1];
			
			model->Draw(encoder);
		}
		[encoder endEncoding];
		
		// reset atomic counter
		*((uint32_t*)[counterbuffer contents] + currentframe) = 0;
		
		// STEP 2: cull lights
		id<MTLComputeCommandEncoder> computeencoder = [commandbuffer computeCommandEncoder];
		
		[computeencoder setBuffer:headbuffer offset:currentframe * headbuffersize atIndex:0];
		[computeencoder setBuffer:nodebuffer offset:currentframe * nodebuffersize atIndex:1];
		[computeencoder setBuffer:lightbuffer offset:currentframe * lightbuffersize atIndex:2];
		[computeencoder setBuffer:counterbuffer offset:currentframe * 4 atIndex:3];
		[computeencoder setBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, lightculluniforms)) atIndex:4];

		[computeencoder setTexture:mtkview.depthStencilTexture atIndex:0];
		
		[computeencoder setComputePipelineState:computestate];
		[computeencoder dispatchThreadgroups:MTLSizeMake(workgroupsx, workgroupsy, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
		[computeencoder endEncoding];

		// STEP 3: accumulate lights
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionStore;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeFront];
			[encoder setRenderPipelineState:forwardstate];
			[encoder setDepthStencilState:zwriteoff];
			
			[encoder setVertexBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, accumvertuniforms)) atIndex:1];
			
			[encoder setFragmentBuffer:headbuffer offset:currentframe * headbuffersize atIndex:0];
			[encoder setFragmentBuffer:nodebuffer offset:currentframe * nodebuffersize atIndex:1];
			[encoder setFragmentBuffer:lightbuffer offset:currentframe * lightbuffersize atIndex:2];
			[encoder setFragmentBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, accumfraguniforms)) atIndex:3];
			
			[encoder setFragmentTexture:ambientcube atIndex:2];
			
			model->Draw(encoder);
		}
		[encoder endEncoding];
		
		// STEP 4: tone mapping
		renderpassdesc.colorAttachments[0].texture = drawable;
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeBack];
			[encoder setRenderPipelineState:tonemapstate];
			[encoder setDepthStencilState:ztestoff];
			
			[encoder setFragmentTexture:rendertarget atIndex:0];
			[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		}
		[encoder endEncoding];
		
		// STEP 5: render flares
		renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.loadAction = MTLLoadActionLoad;
		renderpassdesc.depthAttachment.storeAction = MTLStoreActionDontCare;
		
		encoder = [commandbuffer renderCommandEncoderWithDescriptor:renderpassdesc];
		{
			[encoder setCullMode:MTLCullModeBack];
			[encoder setRenderPipelineState:flaresstate];
			[encoder setDepthStencilState:zwriteoff];
			
			[encoder setVertexBuffer:lightbuffer offset:0 atIndex:0];
			[encoder setVertexBuffer:uniformbuffer offset:(currentframe * sizeof(CombinedUniformData) + offsetof(CombinedUniformData, flaresuniforms)) atIndex:1];
			[encoder setFragmentTexture:flaretexture atIndex:0];
			
			[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:NUM_LIGHTS];
		}
		[encoder endEncoding];

		if (drawtext) {
			// render text
			MTLViewport viewport;
			auto depthattachment = renderpassdesc.depthAttachment;
			auto stencilattachment = renderpassdesc.stencilAttachment;
			
			renderpassdesc.depthAttachment.loadAction = MTLLoadActionDontCare;
			renderpassdesc.depthAttachment = nil;
			renderpassdesc.stencilAttachment = nil;
			
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
		}
		
		[commandbuffer presentDrawable:mtkview.currentDrawable];
		currentframe = (currentframe + 1) % NUM_QUEUED_FRAMES;
	}

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
	app->KeyDownCallback = KeyDown;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;
	
	app->Run();
	delete app;
	
	return 0;
}
