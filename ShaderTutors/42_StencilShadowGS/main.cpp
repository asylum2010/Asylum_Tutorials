
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

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
#include "..\Common\geometryutils.h"

#define MANIFOLD_EPSILON	8e-3f	// to avoid zfight

// helper macros
#define TITLE				"Shader sample 42: Shadow volumes with geometry shader"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = nullptr; } }

// sample classes
class ShadowCaster
{
private:
	Math::Matrix			transform;
	ID3DX10Mesh*			mesh;
	ID3DX10Mesh*			manifold;

public:
	ShadowCaster();
	~ShadowCaster();

	void CreateGeometry(std::function<void (ID3DX10Mesh**, ID3DX10Mesh**)> callback);
	void SetTransform(const Math::Matrix& transform);

	inline const Math::Matrix& GetTransform() const		{ return transform; }
	inline ID3DX10Mesh* GetMesh()						{ return mesh; }
	inline ID3DX10Mesh* GetManifold()					{ return manifold; }
};

// sample variables
Application*				app					= nullptr;
ID3D10Device*				device				= nullptr;
ID3D10RenderTargetView*		defaultview			= nullptr;
ID3D10DepthStencilView*		depthstencilview	= nullptr;
ID3D10Texture2D*			depthstencil		= nullptr;

ID3DX10Mesh*				box					= nullptr;
ID3D10InputLayout*			meshinputlayout		= nullptr;
ID3D10InputLayout*			manifoldinputlayout	= nullptr;

ID3D10ShaderResourceView*	wood				= nullptr;
ID3D10ShaderResourceView*	marble				= nullptr;
ID3D10ShaderResourceView*	helptext			= nullptr;

ID3D10Effect*				zonly				= nullptr;
ID3D10Effect*				blinnphong			= nullptr;
ID3D10Effect*				extrude				= nullptr;
ID3D10Effect*				screenquad			= nullptr;

ID3D10EffectTechnique*		zonlytechnique		= nullptr;
ID3D10EffectTechnique*		blinnphongtechnique	= nullptr;
ID3D10EffectTechnique*		extrudetechnique	= nullptr;
ID3D10EffectTechnique*		silhouettetechnique	= nullptr;
ID3D10EffectTechnique*		screenquadtechnique	= nullptr;

ID3D10BlendState*			noblend				= nullptr;
ID3D10BlendState*			nocolorwrite		= nullptr;
ID3D10BlendState*			alphablend			= nullptr;
ID3D10RasterizerState*		cullmodecw			= nullptr;
ID3D10RasterizerState*		cullmodeccw			= nullptr;
ID3D10RasterizerState*		cullmodenone		= nullptr;
ID3D10DepthStencilState*	depthwriteoff		= nullptr;
ID3D10DepthStencilState*	depthwriteon		= nullptr;
ID3D10DepthStencilState*	carmacksreverse		= nullptr;
ID3D10DepthStencilState*	maskwithstencil		= nullptr;

ShadowCaster*				shadowcasters[3]	= { nullptr };
BasicCamera					camera;
BasicCamera					light;
bool						drawvolume			= false;
bool						drawsilhouette		= false;

// --- Sample classes impl ----------------------------------------------------

ShadowCaster::ShadowCaster()
{
	mesh		= nullptr;
	manifold	= nullptr;
}

ShadowCaster::~ShadowCaster()
{
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(manifold);
}

void ShadowCaster::CreateGeometry(std::function<void (ID3DX10Mesh**, ID3DX10Mesh**)> callback)
{
	callback(&mesh, &manifold);

	manifold->GenerateGSAdjacency();
	manifold->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);
	manifold->CommitToDevice();
}

void ShadowCaster::SetTransform(const Math::Matrix& transform)
{
	this->transform = transform;
}

// --- Sampple functions impl -------------------------------------------------

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

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/simplecolor10.fx", &zonly)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/blinnphong10.fx", &blinnphong)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/shadowvolume10.fx", &extrude)))
		return false;

	if (FAILED(DXCreateEffect(device, "../../Media/ShadersDX/screenquad10.fx", &screenquad)))
		return false;

	zonlytechnique = zonly->GetTechniqueByName("simplecolor");
	blinnphongtechnique = blinnphong->GetTechniqueByName("blinnphong");
	extrudetechnique = extrude->GetTechniqueByName("extrude");
	silhouettetechnique = extrude->GetTechniqueByName("silhouette");
	screenquadtechnique = screenquad->GetTechniqueByName("screenquad");

	if (FAILED(DXLoadMeshFromQM(device, "../../Media/MeshesQM/box.qm", 0, &box)))
		return false;

	box->GetVertexDescription(&decl, &declsize);
	blinnphongtechnique->GetPassByIndex(0)->GetDesc(&passdesc);

	if (FAILED(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &meshinputlayout)))
		return false;

	D3DX10_IMAGE_LOAD_INFO sRGBloadinfo;

	sRGBloadinfo.BindFlags		= D3D10_BIND_SHADER_RESOURCE;
	sRGBloadinfo.CpuAccessFlags	= D3DX10_DEFAULT;
	sRGBloadinfo.Depth			= D3DX10_DEFAULT;
	sRGBloadinfo.Filter			= D3DX10_FILTER_SRGB_IN|D3DX10_FILTER_NONE;
	sRGBloadinfo.FirstMipLevel	= D3DX10_DEFAULT;
	sRGBloadinfo.Format			= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	sRGBloadinfo.Height			= D3DX10_DEFAULT;
	sRGBloadinfo.MipFilter		= D3DX10_DEFAULT;
	sRGBloadinfo.MipLevels		= D3DX10_DEFAULT;
	sRGBloadinfo.MiscFlags		= D3DX10_DEFAULT;
	sRGBloadinfo.pSrcInfo		= nullptr;
	sRGBloadinfo.Usage			= D3D10_USAGE_DEFAULT;
	sRGBloadinfo.Width			= D3DX10_DEFAULT;

	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/wood2.jpg", &sRGBloadinfo, nullptr, &wood, nullptr)))
		return false;

	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, "../../Media/Textures/marble.dds", &sRGBloadinfo, nullptr, &marble, nullptr)))
		return false;

	// setup shadow casters
	Math::Matrix transform;

	D3D10_INPUT_ELEMENT_DESC vertexlayout1[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D10_INPUT_ELEMENT_DESC vertexlayout2[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
	};

	extrudetechnique->GetPassByIndex(0)->GetDesc(&passdesc);

	if (FAILED(device->CreateInputLayout(vertexlayout2, 1, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &manifoldinputlayout)))
		return false;

	shadowcasters[0] = new ShadowCaster();
	shadowcasters[1] = new ShadowCaster();
	shadowcasters[2] = new ShadowCaster();

	Math::MatrixTranslation(transform, -1.5f, 0.55f, -1.2f);
	shadowcasters[0]->SetTransform(transform);

	Math::MatrixTranslation(transform, 1.0f, 0.55f, -0.7f);
	shadowcasters[1]->SetTransform(transform);

	Math::MatrixTranslation(transform, 0.0f, 0.55f, 1.0f);
	shadowcasters[2]->SetTransform(transform);

	shadowcasters[0]->CreateGeometry([&](ID3DX10Mesh** mesh, ID3DX10Mesh** manifold) {
		// box
		D3DX10CreateMesh(device, vertexlayout1, 3, "POSITION", 24, 12, D3DX10_MESH_32_BIT, mesh);
		D3DX10CreateMesh(device, vertexlayout2, 1, "POSITION", 8, 12, D3DX10_MESH_32_BIT, manifold);

		ID3DX10MeshBuffer*				vertexbuffer	= nullptr;
		ID3DX10MeshBuffer*				indexbuffer		= nullptr;
		GeometryUtils::CommonVertex*	vdata1			= nullptr;
		GeometryUtils::PositionVertex*	vdata2			= nullptr;
		uint32_t*						idata			= nullptr;
		SIZE_T							unused;

		(*mesh)->GetVertexBuffer(0, &vertexbuffer);
		(*mesh)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata1, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::CreateBox(vdata1, idata, 1, 1, 1);
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*mesh)->CommitToDevice();

		(*manifold)->GetVertexBuffer(0, &vertexbuffer);
		(*manifold)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata2, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::Create2MBox(vdata2, idata, (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON));
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*manifold)->CommitToDevice();
	});

	shadowcasters[1]->CreateGeometry([&](ID3DX10Mesh** mesh, ID3DX10Mesh** manifold) {
		// sphere
		uint32_t numverts, numinds, num2mverts, num2minds;

		GeometryUtils::NumVerticesIndicesSphere(numverts, numinds, 32, 32);
		GeometryUtils::NumVerticesIndices2MSphere(num2mverts, num2minds, 32, 32);

		D3DX10CreateMesh(device, vertexlayout1, 3, "POSITION", numverts, numinds / 3, D3DX10_MESH_32_BIT, mesh);
		D3DX10CreateMesh(device, vertexlayout2, 1, "POSITION", num2mverts, num2minds / 3, D3DX10_MESH_32_BIT, manifold);

		ID3DX10MeshBuffer*				vertexbuffer	= nullptr;
		ID3DX10MeshBuffer*				indexbuffer		= nullptr;
		GeometryUtils::CommonVertex*	vdata1			= nullptr;
		GeometryUtils::PositionVertex*	vdata2			= nullptr;
		uint32_t*						idata			= nullptr;
		SIZE_T							unused;

		(*mesh)->GetVertexBuffer(0, &vertexbuffer);
		(*mesh)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata1, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::CreateSphere(vdata1, idata, 0.5f, 32, 32);
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*mesh)->CommitToDevice();

		(*manifold)->GetVertexBuffer(0, &vertexbuffer);
		(*manifold)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata2, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::Create2MSphere(vdata2, idata, 0.5f - MANIFOLD_EPSILON, 32, 32);
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*manifold)->CommitToDevice();
	});

	shadowcasters[2]->CreateGeometry([&](ID3DX10Mesh** mesh, ID3DX10Mesh** manifold) {
		// L-shape
		D3DX10CreateMesh(device, vertexlayout1, 3, "POSITION", 36, 20, D3DX10_MESH_32_BIT, mesh);
		D3DX10CreateMesh(device, vertexlayout2, 1, "POSITION", 12, 20, D3DX10_MESH_32_BIT, manifold);

		ID3DX10MeshBuffer*				vertexbuffer	= nullptr;
		ID3DX10MeshBuffer*				indexbuffer		= nullptr;
		GeometryUtils::CommonVertex*	vdata1			= nullptr;
		GeometryUtils::PositionVertex*	vdata2			= nullptr;
		uint32_t*						idata			= nullptr;
		SIZE_T							unused;

		(*mesh)->GetVertexBuffer(0, &vertexbuffer);
		(*mesh)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata1, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::CreateLShape(vdata1, idata, 1.5f, 1.0f, 1, 1.2f, 0.6f);
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*mesh)->CommitToDevice();

		(*manifold)->GetVertexBuffer(0, &vertexbuffer);
		(*manifold)->GetIndexBuffer(&indexbuffer);

		vertexbuffer->Map((void**)&vdata2, &unused);
		indexbuffer->Map((void**)&idata, &unused);
		{
			GeometryUtils::Create2MLShape(vdata2, idata, 1.5f - MANIFOLD_EPSILON, 1.0f, 1 - MANIFOLD_EPSILON, 1.2f - MANIFOLD_EPSILON, 0.6f);
		}
		indexbuffer->Unmap();
		vertexbuffer->Unmap();

		vertexbuffer->Release();
		indexbuffer->Release();

		(*manifold)->CommitToDevice();
	});

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

	rasterdesc.CullMode = D3D10_CULL_NONE;
	device->CreateRasterizerState(&rasterdesc, &cullmodenone);

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

	depthdesc.DepthEnable					= TRUE;
	depthdesc.StencilEnable					= TRUE;
	depthdesc.DepthWriteMask				= D3D10_DEPTH_WRITE_MASK_ZERO;
	depthdesc.FrontFace.StencilDepthFailOp	= D3D10_STENCIL_OP_DECR;
	depthdesc.BackFace.StencilDepthFailOp	= D3D10_STENCIL_OP_INCR;

	device->CreateDepthStencilState(&depthdesc, &carmacksreverse);

	depthdesc.DepthFunc						= D3D10_COMPARISON_LESS_EQUAL;
	depthdesc.FrontFace.StencilFunc			= D3D10_COMPARISON_EQUAL;
	depthdesc.FrontFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilFunc			= D3D10_COMPARISON_EQUAL;
	depthdesc.BackFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;

	device->CreateDepthStencilState(&depthdesc, &maskwithstencil);

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

	blenddesc.RenderTargetWriteMask[0] = 0;
	device->CreateBlendState(&blenddesc, &nocolorwrite);

	// render text
	if (FAILED(DXRenderTextEx(device,
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate light\n\n1 - Toggle silhouette\n2 - Toggle shadow volume",
		512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
	{
		return false;
	}

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(45), Math::DegreesToRadians(45), 0);

	// setup light
	light.SetDistance(10);
	light.SetPosition(0, 0, 0);
	light.SetOrientation(Math::DegreesToRadians(153), Math::DegreesToRadians(45), 0);
	light.SetPitchLimits(0.1f, 1.45f);

	return true;
}

void UninitScene()
{
	delete shadowcasters[0];
	delete shadowcasters[1];
	delete shadowcasters[2];

	SAFE_RELEASE(defaultview);
	SAFE_RELEASE(depthstencilview);
	SAFE_RELEASE(depthstencil);

	SAFE_RELEASE(box);
	SAFE_RELEASE(meshinputlayout);
	SAFE_RELEASE(manifoldinputlayout);

	SAFE_RELEASE(wood);
	SAFE_RELEASE(marble);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(zonly);
	SAFE_RELEASE(blinnphong);
	SAFE_RELEASE(extrude);
	SAFE_RELEASE(screenquad);

	SAFE_RELEASE(noblend);
	SAFE_RELEASE(nocolorwrite);
	SAFE_RELEASE(alphablend);
	SAFE_RELEASE(cullmodecw);
	SAFE_RELEASE(cullmodeccw);
	SAFE_RELEASE(cullmodenone);
	SAFE_RELEASE(depthwriteoff);
	SAFE_RELEASE(depthwriteon);
	SAFE_RELEASE(carmacksreverse);
	SAFE_RELEASE(maskwithstencil);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		drawsilhouette = !drawsilhouette;
		break;

	case KeyCode2:
		drawvolume = !drawvolume;
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
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	} else if (state & MouseButtonRight) {
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
	light.Update(dt);
}

void RenderScene(ID3D10Effect* effect, ID3D10EffectTechnique* technique)
{
	Math::Matrix world, worldinv;
	Math::Vector4 uv(3, 3, 0, 0);

	Math::MatrixScaling(world, 5, 0.1f, 5);
	Math::MatrixInverse(worldinv, world);

	device->RSSetState(cullmodecw);

	effect->GetVariableByName("matWorld")->AsMatrix()->SetMatrix(&world._11);
	effect->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix(&worldinv._11);
	effect->GetVariableByName("uv")->AsVector()->SetFloatVector(&uv.x);
	effect->GetVariableByName("baseColor")->AsShaderResource()->SetResource(wood);

	technique->GetPassByIndex(0)->Apply(0);
	box->DrawSubset(0);

	if (!drawsilhouette) {
		uv.x = uv.y = 1;

		effect->GetVariableByName("uv")->AsVector()->SetFloatVector(&uv.x);
		effect->GetVariableByName("baseColor")->AsShaderResource()->SetResource(marble);

		for (int i = 0; i < 3; ++i) {
			world = shadowcasters[i]->GetTransform();
			Math::MatrixInverse(worldinv, world);

			effect->GetVariableByName("matWorld")->AsMatrix()->SetMatrix(&world._11);
			effect->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix(&worldinv._11);

			technique->GetPassByIndex(0)->Apply(0);
			shadowcasters[i]->GetMesh()->DrawSubset(0);
		}
	}

	device->RSSetState(cullmodeccw);
}

void DrawShadowVolumes(ID3D10EffectTechnique* technique)
{
	for (int i = 0; i < 3; ++i) {
		ShadowCaster* caster = shadowcasters[i];
		Math::Matrix world = caster->GetTransform();

		extrude->GetVariableByName("matWorld")->AsMatrix()->SetMatrix(&world._11);

		technique->GetPassByIndex(0)->Apply(0);
		caster->GetManifold()->DrawSubset(0);
	}
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix	view, proj;
	Math::Matrix	viewproj;
	Math::Vector4	eye;
	Math::Vector4	lightpos(0, 0, 0, 1);
	float			clearcolor[4] = { 0.0f, 0.0103f, 0.0707f, 1.0f };

	light.Animate(alpha);
	light.GetEyePosition((Math::Vector3&)lightpos);

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);
	
	// put far plane to infinity
	proj._33 = -1;
	proj._43 = -camera.GetNearPlane();

	Math::MatrixMultiply(viewproj, view, proj);

	// render
	device->OMSetRenderTargets(1, &defaultview, depthstencilview);

	device->ClearRenderTargetView(defaultview, clearcolor);
	device->ClearDepthStencilView(depthstencilview, D3D10_CLEAR_DEPTH|D3D10_CLEAR_STENCIL, 1.0f, 0);
	{
		Math::Vector4 black(0, 0, 0, 1);
		Math::Vector4 yellow(1, 1, 0, 0.5f);

		// STEP 1: z-only pass
		zonly->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix(&viewproj._11);
		zonly->GetVariableByName("matColor")->AsVector()->SetFloatVector(&black.x);

		device->IASetInputLayout(manifoldinputlayout);
		device->OMSetDepthStencilState(depthwriteon, 0);
		device->OMSetBlendState(noblend, NULL, 0xffffffff);

		RenderScene(zonly, zonlytechnique);

		// STEP 2: fill stencil buffer
		extrude->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		extrude->GetVariableByName("lightPos")->AsVector()->SetFloatVector((float*)&lightpos.x);

		device->OMSetDepthStencilState(carmacksreverse, 0);
		device->OMSetBlendState(nocolorwrite, NULL, 0xffffffff);
		device->RSSetState(cullmodenone);

		DrawShadowVolumes(extrudetechnique);

		// STEP 3: render scene
		blinnphong->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix(&viewproj._11);
		blinnphong->GetVariableByName("eyePos")->AsVector()->SetFloatVector(&eye.x);
		blinnphong->GetVariableByName("lightPos")->AsVector()->SetFloatVector(&lightpos.x);
		blinnphong->GetVariableByName("lightAmbient")->AsVector()->SetFloatVector(&black.x);

		device->IASetInputLayout(meshinputlayout);
		device->OMSetDepthStencilState(maskwithstencil, 0);
		device->OMSetBlendState(noblend, NULL, 0xffffffff);

		RenderScene(blinnphong, blinnphongtechnique);

		if (drawsilhouette) {
			extrude->GetVariableByName("matColor")->AsVector()->SetFloatVector(&yellow.x);

			device->IASetInputLayout(manifoldinputlayout);
			device->RSSetState(cullmodecw);
			device->OMSetDepthStencilState(depthwriteon, 0);
			device->OMSetBlendState(noblend, NULL, 0xffffffff);

			DrawShadowVolumes(silhouettetechnique);
		}

		if (drawvolume) {
			extrude->GetVariableByName("matColor")->AsVector()->SetFloatVector(&yellow.x);

			device->IASetInputLayout(manifoldinputlayout);
			device->RSSetState(cullmodecw);
			device->OMSetDepthStencilState(depthwriteon, 0);
			device->OMSetBlendState(alphablend, NULL, 0xffffffff);

			DrawShadowVolumes(extrudetechnique);
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
	app->MouseScrollCallback = MouseScroll;

	app->Run();
	delete app;

	return 0;
}
