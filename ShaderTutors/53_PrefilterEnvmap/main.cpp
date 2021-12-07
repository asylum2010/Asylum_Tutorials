
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

//#define DEBUG_CAPTURE

#include <d3d10.h>
#include <d3dx10.h>

#ifdef DEBUG_CAPTURE
#	pragma comment(lib, "dxgi.lib")

#	include <DXGIType.h>
#	include <dxgi1_2.h>
#	include <dxgi1_3.h>
#	include <DXProgrammableCapture.h>
#endif

#include "..\Common\application.h"
#include "..\Common\dx10ext.h"
#include "..\Common\basiccamera.h"

#define FILENAME			"uffizi"
#define NUM_SPHERES			11
#define SPHERE_SPACING		0.2f

// helper macros
#define TITLE				"Preintegrated irradiance maps"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = nullptr; } }

// sample variables
Application*				app					= nullptr;
ID3D10Device*				device				= nullptr;
ID3D10RenderTargetView*		defaultview			= nullptr;
ID3D10DepthStencilView*		depthstencilview	= nullptr;
ID3D10Texture2D*			depthstencil		= nullptr;

ID3DX10Mesh*				sky					= nullptr;
ID3DX10Mesh*				sphere				= nullptr;
ID3D10InputLayout*			skyinputlayout		= nullptr;
ID3D10InputLayout*			sphereinputlayout	= nullptr;

ID3D10ShaderResourceView*	environment			= nullptr;
ID3D10ShaderResourceView*	integrateddiff		= nullptr;
ID3D10ShaderResourceView*	integratedspec		= nullptr;
ID3D10ShaderResourceView*	integratedbrdf		= nullptr;
ID3D10ShaderResourceView*	helptext			= nullptr;

ID3D10Effect*				skyeffect			= nullptr;
ID3D10Effect*				metaleffect			= nullptr;
ID3D10Effect*				insulatoreffect		= nullptr;
ID3D10Effect*				screenquad			= nullptr;
ID3D10Effect*				effect				= nullptr;

ID3D10EffectTechnique*		skytechnique		= nullptr;
ID3D10EffectTechnique*		metaltechnique		= nullptr;
ID3D10EffectTechnique*		insulatortechnique	= nullptr;
ID3D10EffectTechnique*		screenquadtechnique	= nullptr;
ID3D10EffectTechnique*		technique			= nullptr;

ID3D10BlendState*			noblend				= nullptr;
ID3D10BlendState*			alphablend			= nullptr;
ID3D10RasterizerState*		cullmodecw			= nullptr;
ID3D10RasterizerState*		cullmodeccw			= nullptr;
ID3D10DepthStencilState*	depthwriteoff		= nullptr;
ID3D10DepthStencilState*	depthwriteon		= nullptr;

BasicCamera					camera;

static void Prefilter()
{
	const UINT MaxCubemapDiffSize = 128;
	const UINT MaxCubemapSpecSize = 512;
	const UINT MaxCubemapSpecMips = 8;

	D3D10_RENDER_TARGET_VIEW_DESC	rtvdesc;
	D3D10_TEXTURE2D_DESC			envdesc;
	D3D10_TEXTURE2D_DESC			texdesc;
	D3D10_VIEWPORT					viewport;

	D3DXMATRIX						views[6];
	D3DXMATRIX						proj;
	D3DXVECTOR3						look, up, eye(0, 0, 0);
	float							clearcolor[4] = { 1, 0, 0, 1 };

	ID3D10RenderTargetView*			cubertvs[MaxCubemapSpecMips] = { nullptr };
	ID3D10Texture2D*				cubemap		= nullptr;
	ID3D10Effect*					prefilter	= nullptr;
	ID3D10EffectTechnique*			technique	= nullptr;

	// view matrices
	up = D3DXVECTOR3(0, 1, 0);
	look = D3DXVECTOR3(1, 0, 0);
	D3DXMatrixLookAtLH(&views[0], &eye, &look, &up);

	look = D3DXVECTOR3(-1, 0, 0);
	D3DXMatrixLookAtLH(&views[1], &eye, &look, &up);

	look = D3DXVECTOR3(0, 0, 1);
	D3DXMatrixLookAtLH(&views[4], &eye, &look, &up);

	look = D3DXVECTOR3(0, 0, -1);
	D3DXMatrixLookAtLH(&views[5], &eye, &look, &up);

	up = D3DXVECTOR3(0, 0, -1);
	look = D3DXVECTOR3(0, 1, 0);
	D3DXMatrixLookAtLH(&views[2], &eye, &look, &up);

	up = D3DXVECTOR3(0, 0, 1);
	look = D3DXVECTOR3(0, -1, 0);
	D3DXMatrixLookAtLH(&views[3], &eye, &look, &up);

	// projection matrix
	D3DXMatrixPerspectiveFovLH(&proj, (float)D3DX_PI * 0.5f, 1.0f, 0.1f, 10.0f);

	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;

	// load shader
	std::cout << "Optimizing shaders...\n";

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/prefilterenvmap10.fx", &prefilter)))
		return;

	// render targets
	environment->GetResource((ID3D10Resource**)&cubemap);
	
	cubemap->GetDesc(&envdesc);
	cubemap->Release();

	if (envdesc.MipLevels > MaxCubemapSpecMips)
		envdesc.MipLevels = MaxCubemapSpecMips;

	if (envdesc.Width > MaxCubemapSpecSize)
		envdesc.Width = MaxCubemapSpecSize;

	if (envdesc.Height > MaxCubemapSpecSize)
		envdesc.Height = MaxCubemapSpecSize;

	texdesc.Width				= envdesc.Width;
	texdesc.Height				= envdesc.Height;
	texdesc.MipLevels			= 0;
	texdesc.ArraySize			= 6;
	texdesc.SampleDesc.Count	= 1;
	texdesc.SampleDesc.Quality	= 0;
	texdesc.Usage				= D3D10_USAGE_DEFAULT;
	texdesc.CPUAccessFlags		= 0;
	texdesc.Format				= DXGI_FORMAT_R16G16B16A16_FLOAT;
	texdesc.BindFlags			= D3D10_BIND_RENDER_TARGET|D3D10_BIND_SHADER_RESOURCE;
	texdesc.MiscFlags			= D3D10_RESOURCE_MISC_GENERATE_MIPS|D3D10_RESOURCE_MISC_TEXTURECUBE;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &cubemap))) {
		std::cout << "Could not create render target\n";
		return;
	}

	rtvdesc.Format							= texdesc.Format;
	rtvdesc.ViewDimension					= D3D10_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvdesc.Texture2DArray.FirstArraySlice	= 0;
	rtvdesc.Texture2DArray.ArraySize		= 6;

	for (UINT i = 0; i < envdesc.MipLevels; ++i) {
		rtvdesc.Texture2DArray.MipSlice = i;

		if (FAILED(device->CreateRenderTargetView(cubemap, &rtvdesc, &cubertvs[i]))) {
			std::cout << "Could not create render target view\n";
			return;
		}
	}

	// render specular irradiance
	prefilter->GetVariableByName("environment")->AsShaderResource()->SetResource(environment);
	prefilter->GetVariableByName("matCubeViews")->AsMatrix()->SetMatrixArray((float*)&views[0], 0, 6);
	prefilter->GetVariableByName("matProj")->AsMatrix()->SetMatrix(&proj._11);

	technique = prefilter->GetTechniqueByName("prefilter_spec");

#ifdef DEBUG_CAPTURE
	IDXGraphicsAnalysis* analysis = nullptr;
	DXGIGetDebugInterface1(0, __uuidof(analysis), (void**)&analysis);

	analysis->BeginCapture();
#endif

	for (UINT i = 0; i < envdesc.MipLevels; ++i) {
		float roughness = (i + 0.5f) / envdesc.MipLevels;
		viewport.Width = viewport.Height = Math::Max<UINT>(1, texdesc.Width / (1 << i));

		prefilter->GetVariableByName("roughness")->AsScalar()->SetFloat(roughness);

		device->OMSetRenderTargets(1, &cubertvs[i], NULL);
		device->RSSetViewports(1, &viewport);
		device->ClearRenderTargetView(cubertvs[i], clearcolor);
		{
			device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			device->IASetInputLayout(skyinputlayout);
			device->OMSetDepthStencilState(depthwriteoff, 0);

			technique->GetPassByIndex(0)->Apply(0);
			sky->DrawSubset(0);
		}
	}

#ifdef DEBUG_CAPTURE
	analysis->EndCapture();
	analysis->Release();
#endif

	// save & clean up
	D3DX10SaveTextureToFile(cubemap, D3DX10_IFF_DDS, "../../Media/Textures/" FILENAME "_spec_irrad.dds");

	for (UINT i = 0; i < envdesc.MipLevels; ++i) {
		SAFE_RELEASE(cubertvs[i]);
	}

	SAFE_RELEASE(cubemap);

	// now generate diffuse irradiance
	technique = prefilter->GetTechniqueByName("prefilter_diff");

	if (envdesc.Width > MaxCubemapDiffSize)
		envdesc.Width = MaxCubemapDiffSize;

	if (envdesc.Height > MaxCubemapDiffSize)
		envdesc.Height = MaxCubemapDiffSize;
	
	texdesc.Width		= envdesc.Width;
	texdesc.Height		= envdesc.Height;
	texdesc.MipLevels	= 1;
	texdesc.BindFlags	= D3D10_BIND_RENDER_TARGET;
	texdesc.MiscFlags	= D3D10_RESOURCE_MISC_TEXTURECUBE;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &cubemap))) {
		std::cout << "Could not create render target\n";
		return;
	}

	rtvdesc.Texture2DArray.MipSlice = 0;

	if (FAILED(device->CreateRenderTargetView(cubemap, &rtvdesc, &cubertvs[0]))) {
		std::cout << "Could not create render target view\n";
		return;
	}

	viewport.Width = viewport.Height = texdesc.Width;

	device->OMSetRenderTargets(1, &cubertvs[0], NULL);
	device->RSSetViewports(1, &viewport);
	device->ClearRenderTargetView(cubertvs[0], clearcolor);
	{
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device->IASetInputLayout(skyinputlayout);
		device->OMSetDepthStencilState(depthwriteoff, 0);

		technique->GetPassByIndex(0)->Apply(0);
		sky->DrawSubset(0);
	}

	// save & clean up
	D3DX10SaveTextureToFile(cubemap, D3DX10_IFF_DDS, "../../Media/Textures/" FILENAME "_diff_irrad.dds");

	SAFE_RELEASE(cubertvs[0]);
	SAFE_RELEASE(cubemap);
	SAFE_RELEASE(prefilter);

	// generate BRDF lookup texture
	ID3D10Texture2D*		brdftex			= nullptr;
	ID3D10RenderTargetView*	brdfrtv			= nullptr;
	ID3D10Effect*			integratebrdf	= nullptr;

	std::cout << "Generating BRDF texture...\n";

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/integratebrdf10.fx", &integratebrdf)))
		return;

	technique = integratebrdf->GetTechniqueByName("integratebrdf");

	texdesc.Width				= 256;
	texdesc.Height				= 256;
	texdesc.MipLevels			= 1;
	texdesc.ArraySize			= 1;
	texdesc.Format				= DXGI_FORMAT_R16G16_FLOAT;
	texdesc.MiscFlags			= 0;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &brdftex))) {
		std::cout << "Could not create render target\n";
		return;
	}

	rtvdesc.Format				= texdesc.Format;
	rtvdesc.ViewDimension		= D3D10_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice	= 0;

	if (FAILED(device->CreateRenderTargetView(brdftex, &rtvdesc, &brdfrtv))) {
		std::cout << "Could not create render target view\n";
		return;
	}

	viewport.Width = viewport.Height = texdesc.Width;

	device->OMSetRenderTargets(1, &brdfrtv, NULL);
	device->RSSetViewports(1, &viewport);
	device->ClearRenderTargetView(brdfrtv, clearcolor);
	{
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		device->OMSetDepthStencilState(depthwriteoff, 0);

		technique->GetPassByIndex(0)->Apply(0);
		device->Draw(4, 0);
	}

	// save & clean up
	D3DX10SaveTextureToFile(brdftex, D3DX10_IFF_DDS, "../../Media/Textures/brdf.dds");

	SAFE_RELEASE(brdfrtv);
	SAFE_RELEASE(brdftex);
	SAFE_RELEASE(integratebrdf);

	// reset state
	viewport.Width = app->GetClientWidth();
	viewport.Height = app->GetClientHeight();

	device->RSSetViewports(1, &viewport);
}

bool InitScene()
{
	// NOTE: must create depth-stencil manually
	D3D10_TEXTURE2D_DESC			depthstencildesc;
	D3D10_DEPTH_STENCIL_VIEW_DESC	depthstencilviewdesc;
	uint32_t						screenwidth		= app->GetClientWidth();
	uint32_t						screenheight	= app->GetClientHeight();
	IDXGISwapChain*					swapchain		= (IDXGISwapChain*)app->GetSwapChain();
	ID3D10Texture2D*				backbuffer		= nullptr;

	memset(&depthstencildesc, 0, sizeof(D3D10_TEXTURE2D_DESC));
	memset(&depthstencilviewdesc, 0, sizeof(D3D10_DEPTH_STENCIL_VIEW_DESC));

	if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&backbuffer)))
		return false;

	if (FAILED(device->CreateRenderTargetView(backbuffer, NULL, &defaultview))) {
		backbuffer->Release();
		return false;
	}

	backbuffer->Release();

	depthstencildesc.Width					= screenwidth;
	depthstencildesc.Height					= screenheight;
	depthstencildesc.MipLevels				= 1;
	depthstencildesc.ArraySize				= 1;
	depthstencildesc.Format					= DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthstencildesc.SampleDesc.Count		= 1;
	depthstencildesc.SampleDesc.Quality		= 0;
	depthstencildesc.Usage					= D3D10_USAGE_DEFAULT;
	depthstencildesc.BindFlags				= D3D10_BIND_DEPTH_STENCIL;

	if (FAILED(device->CreateTexture2D(&depthstencildesc, NULL, &depthstencil)))
		return false;

	depthstencilviewdesc.Format				= depthstencildesc.Format;
	depthstencilviewdesc.ViewDimension		= D3D10_DSV_DIMENSION_TEXTURE2D;
	depthstencilviewdesc.Texture2D.MipSlice	= 0;

	if (FAILED(device->CreateDepthStencilView(depthstencil, &depthstencilviewdesc, &depthstencilview)))
		return false;

	// now can create resources
	D3D10_PASS_DESC					passdesc;
	const D3D10_INPUT_ELEMENT_DESC*	decl = nullptr;
	UINT							declsize = 0;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/sky10.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/metal10.fx", &metaleffect)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/insulator10.fx", &insulatoreffect)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/screenquad10.fx", &screenquad)))
		return false;

	skytechnique = skyeffect->GetTechniqueByName("sky");
	metaltechnique = metaleffect->GetTechniqueByName("metal");
	insulatortechnique = insulatoreffect->GetTechniqueByName("insulator");
	screenquadtechnique = screenquad->GetTechniqueByName("screenquad");

	if (FAILED(DXLoadMeshFromQM(device, "../../Media/MeshesQM/sky.qm", 0, &sky)))
		return false;

	if (FAILED(DXLoadMeshFromQM(device, "../../Media/MeshesQM/sphere.qm", 0, &sphere)))
		return false;

	sky->GetVertexDescription(&decl, &declsize);
	skytechnique->GetPassByIndex(0)->GetDesc(&passdesc);

	if (FAILED(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &skyinputlayout)))
		return false;

	sphere->GetVertexDescription(&decl, &declsize);
	metaltechnique->GetPassByIndex(0)->GetDesc(&passdesc);

	if (FAILED(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &sphereinputlayout)))
		return false;

	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/" FILENAME ".dds", 0, 0, &environment, 0)))
		return false;

	// create pipeline states
	D3D10_RASTERIZER_DESC rasterdesc;
	D3D10_DEPTH_STENCIL_DESC depthdesc;
	D3D10_BLEND_DESC blenddesc;

	// rasterizer state
	rasterdesc.FillMode						= D3D10_FILL_SOLID;
	rasterdesc.CullMode						= D3D10_CULL_BACK;
	rasterdesc.FrontCounterClockwise		= FALSE;
	rasterdesc.DepthBias					= 0;
	rasterdesc.DepthBiasClamp				= 0;
	rasterdesc.SlopeScaledDepthBias			= 0;
	rasterdesc.DepthClipEnable				= TRUE;
	rasterdesc.ScissorEnable				= FALSE;
	rasterdesc.MultisampleEnable			= FALSE;
	rasterdesc.AntialiasedLineEnable		= FALSE;

	device->CreateRasterizerState(&rasterdesc, &cullmodeccw);

	rasterdesc.FrontCounterClockwise = TRUE;
	device->CreateRasterizerState(&rasterdesc, &cullmodecw);

	// depth-stencil state
	depthdesc.DepthEnable					= TRUE;
	depthdesc.DepthWriteMask				= D3D10_DEPTH_WRITE_MASK_ALL;
	depthdesc.DepthFunc						= D3D10_COMPARISON_LESS;
	depthdesc.StencilEnable					= FALSE;
	depthdesc.StencilReadMask				= D3D10_DEFAULT_STENCIL_READ_MASK;
	depthdesc.StencilWriteMask				= D3D10_DEFAULT_STENCIL_WRITE_MASK;
	
	depthdesc.FrontFace.StencilFailOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.FrontFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;
	depthdesc.FrontFace.StencilPassOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.FrontFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;
	depthdesc.BackFace.StencilFailOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilPassOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&depthdesc, &depthwriteon);

	depthdesc.DepthEnable = FALSE;
	device->CreateDepthStencilState(&depthdesc, &depthwriteoff);

	// blend state
	for (int i = 0; i < 8; ++i) {
		blenddesc.RenderTargetWriteMask[i] = 0;
		blenddesc.BlendEnable[i] = FALSE;
	}

	blenddesc.RenderTargetWriteMask[0]	= D3D10_COLOR_WRITE_ENABLE_ALL;
	blenddesc.BlendEnable[0]			= TRUE;
	blenddesc.AlphaToCoverageEnable		= FALSE;
	blenddesc.BlendOp					= D3D10_BLEND_OP_ADD;
	blenddesc.BlendOpAlpha				= D3D10_BLEND_OP_ADD;
	blenddesc.DestBlend					= D3D10_BLEND_INV_SRC_ALPHA;
	blenddesc.DestBlendAlpha			= D3D10_BLEND_INV_SRC_ALPHA;
	blenddesc.SrcBlend					= D3D10_BLEND_SRC_ALPHA;
	blenddesc.SrcBlendAlpha				= D3D10_BLEND_SRC_ALPHA;

	device->CreateBlendState(&blenddesc, &alphablend);

	blenddesc.BlendEnable[0] = FALSE;
	device->CreateBlendState(&blenddesc, &noblend);

	// render text
	if (FAILED(DXRenderTextEx(device, "1 - Conductor (metal)\n2 - Insulator (plastic)", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// setup camera
	camera.SetAspect((float)app->GetClientWidth() / (float)app->GetClientHeight());
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 30);

	float tfovx = camera.GetAspect() * tanf(camera.GetFov() * 0.5f);
	camera.SetDistance((NUM_SPHERES * 0.5f + SPHERE_SPACING * (NUM_SPHERES + 1) * 0.5f) / tfovx);

	// prefilter environment map
	Prefilter();

	// load result
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/" FILENAME "_diff_irrad.dds", 0, 0, &integrateddiff, 0)))
		return false;

	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/" FILENAME "_spec_irrad.dds", 0, 0, &integratedspec, 0)))
		return false;

	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/brdf.dds", 0, 0, &integratedbrdf, 0)))
		return false;

	effect = metaleffect;
	technique = metaltechnique;

	return true;
}

void UninitScene()
{
	SAFE_RELEASE(defaultview);
	SAFE_RELEASE(depthstencilview);
	SAFE_RELEASE(depthstencil);

	SAFE_RELEASE(sky);
	SAFE_RELEASE(sphere);

	SAFE_RELEASE(skyinputlayout);
	SAFE_RELEASE(sphereinputlayout);

	SAFE_RELEASE(environment);
	SAFE_RELEASE(integrateddiff);
	SAFE_RELEASE(integratedspec);
	SAFE_RELEASE(integratedbrdf);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(metaleffect);
	SAFE_RELEASE(insulatoreffect);
	SAFE_RELEASE(screenquad);

	SAFE_RELEASE(noblend);
	SAFE_RELEASE(alphablend);
	SAFE_RELEASE(cullmodecw);
	SAFE_RELEASE(cullmodeccw);
	SAFE_RELEASE(depthwriteoff);
	SAFE_RELEASE(depthwriteon);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		effect = metaleffect;
		technique = metaltechnique;
		break;

	case KeyCode2:
		effect = insulatoreffect;
		technique = insulatortechnique;
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

	if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	}
}

void Update(float dt)
{
	camera.Update(dt);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	Math::Matrix	world, worldinv, view, proj;
	Math::Matrix	skyview, viewproj;
	Math::Vector4	eye;
	float			clearcolor[4] = { 0.0f, 0.0103f, 0.0707f, 1.0f };

	time += elapsedtime;

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);
	
	device->OMSetRenderTargets(1, &defaultview, depthstencilview);

	device->ClearRenderTargetView(defaultview, clearcolor);
	device->ClearDepthStencilView(depthstencilview, D3D10_CLEAR_DEPTH|D3D10_CLEAR_STENCIL, 1.0f, 0);
	{
		// render sky
		skyview = view;
		skyview._41 = skyview._42 = skyview._43 = 0;

		Math::MatrixIdentity(world);
		Math::MatrixMultiply(viewproj, skyview, proj);

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device->IASetInputLayout(skyinputlayout);
		device->RSSetState(cullmodecw);
		device->OMSetDepthStencilState(depthwriteoff, 0);

		skyeffect->GetVariableByName("environment")->AsShaderResource()->SetResource(environment);
		skyeffect->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix(&viewproj._11);
		skyeffect->GetVariableByName("matSkyRotation")->AsMatrix()->SetMatrix((float*)&world);

		skytechnique->GetPassByIndex(0)->Apply(0);
		sky->DrawSubset(0);

		// render spheres
		device->IASetInputLayout(sphereinputlayout);
		device->OMSetDepthStencilState(depthwriteon, 0);

		Math::MatrixIdentity(world);
		Math::MatrixMultiply(viewproj, view, proj);

		for (int i = 0; i < NUM_SPHERES; ++i) {
			float roughness = (float)i / (NUM_SPHERES - 1);
			world._41 = (i - (NUM_SPHERES - 1) * 0.5f) - ((NUM_SPHERES - 1) * 0.5f) * SPHERE_SPACING + i * SPHERE_SPACING;

			Math::MatrixInverse(worldinv, world);

			if (effect == metaleffect) {
				effect->GetVariableByName("irradiance")->AsShaderResource()->SetResource(integratedspec);
			} else {
				effect->GetVariableByName("irradiance1")->AsShaderResource()->SetResource(integrateddiff);
				effect->GetVariableByName("irradiance2")->AsShaderResource()->SetResource(integratedspec);
			}

			effect->GetVariableByName("brdfLUT")->AsShaderResource()->SetResource(integratedbrdf);

			effect->GetVariableByName("matWorld")->AsMatrix()->SetMatrix(&world._11);
			effect->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix(&worldinv._11);
			effect->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix(&viewproj._11);

			effect->GetVariableByName("eyePos")->AsVector()->SetFloatVector(&eye.x);
			effect->GetVariableByName("roughness")->AsScalar()->SetFloat(roughness);

			technique->GetPassByIndex(0)->Apply(0);
			sphere->DrawSubset(0);
		}

		// render text
		D3D10_VIEWPORT viewport;

		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.TopLeftX	= 10;
		viewport.TopLeftY	= 10;
		viewport.Width		= 512;
		viewport.Height		= 512;

		screenquad->GetVariableByName("image")->AsShaderResource()->SetResource(helptext);

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		device->RSSetViewports(1, &viewport);
		device->RSSetState(cullmodeccw);
		device->OMSetDepthStencilState(depthwriteoff, 0);
		device->OMSetBlendState(alphablend, NULL, 0xffffffff);

		screenquadtechnique->GetPassByIndex(0)->Apply(0);
		device->Draw(4, 0);

		// reset
		viewport.TopLeftX	= 0;
		viewport.TopLeftY	= 0;
		viewport.Width		= app->GetClientWidth();
		viewport.Height		= app->GetClientHeight();

		device->RSSetViewports(1, &viewport);
		device->OMSetBlendState(noblend, NULL, 0xffffffff);
	}

	app->Present();
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIDirect3D10)) {
		delete app;
		return 1;
	}

	device = (ID3D10Device*)app->GetLogicalDevice();

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
