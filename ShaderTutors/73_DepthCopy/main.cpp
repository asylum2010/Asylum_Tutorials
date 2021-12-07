
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

#include "..\Common\dx11ext.h"
#include "..\Common\application.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Sample 73: Direct3D depth copy problem (NVidia)"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

struct UniformData
{
	Math::Matrix world;			// 0
	Math::Matrix worldinv;		// 64
	Math::Matrix viewproj;		// 128

	Math::Vector4 lightpos;		// 192
	Math::Vector4 lightcolor;	// 208
	Math::Vector4 lightambient;	// 224
	Math::Vector4 eye;			// 240
	Math::Vector2 uvscale;		// 256

	float padding[2];			// 264
}; // 272 bytes total

// sample variables
Application*				app					= nullptr;
BasicCamera					camera;
D3D11Mesh*					mesh				= nullptr;
D3D11ScreenQuad*			screenquad			= nullptr;

ID3D11DeviceContext*		context				= nullptr;
ID3D11RenderTargetView*		defaultview			= nullptr;
ID3D11RenderTargetView*		bottomlayerrtv		= nullptr;
ID3D11RenderTargetView*		feedbacklayerrtv	= nullptr;
ID3D11DepthStencilView*		bottomlayerdsv		= nullptr;
ID3D11DepthStencilView*		feedbacklayerdsv	= nullptr;
ID3D11ShaderResourceView*	bottomlayersrv		= nullptr;
ID3D11ShaderResourceView*	feedbacklayersrv	= nullptr;

ID3D11Texture2D*			bottomlayercolor	= nullptr;
ID3D11Texture2D*			feedbacklayercolor	= nullptr;
ID3D11Texture2D*			bottomlayerdepth	= nullptr;
ID3D11Texture2D*			feedbacklayerdepth	= nullptr;

ID3D11VertexShader*			vertexshader		= nullptr;
ID3D11PixelShader*			pixelshader			= nullptr;
ID3D11RasterizerState*		rasterstate			= nullptr;
ID3D11Buffer*				uniformbuffer		= nullptr;
ID3D11Texture2D*			texture				= nullptr;
ID3D11ShaderResourceView*	texturesrv			= nullptr;
ID3D11SamplerState*			linearsampler		= nullptr;

bool InitScene()
{
	context = (ID3D11DeviceContext*)app->GetDeviceContext();

	uint32_t			screenwidth		= app->GetClientWidth();
	uint32_t			screenheight	= app->GetClientHeight();
	ID3D11Device*		device			= (ID3D11Device*)app->GetLogicalDevice();
	IDXGISwapChain*		swapchain		= (IDXGISwapChain*)app->GetSwapChain();
	ID3D11Texture2D*	backbuffer		= nullptr;

	if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer)))
		return false;

	if (FAILED(device->CreateRenderTargetView(backbuffer, NULL, &defaultview))) {
		backbuffer->Release();
		return false;
	}
	
	backbuffer->Release();

	// load mesh
	if (FAILED(DXLoadMeshFromQM(device, "../../Media/MeshesQM/teapot.qm", 0, &mesh))) {
		MYERROR("Could not load mesh");
		return false;
	}

	// compile shaders
	ID3DBlob*	code	= nullptr;
	ID3DBlob*	errors	= nullptr;
	UINT		flags	= D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		L"../../Media/ShadersDX11/blinnphong11.hlsl",
		NULL, NULL, "vs_main", "vs_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();

		return false;
	}

	if (FAILED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &vertexshader))) {
		MYERROR("Could not create vertex shader");
		return false;
	}

	if (!mesh->LinkToVertexShader(code->GetBufferPointer(), code->GetBufferSize())) {
		MYERROR("Vertex shader linkage failed");
		return false;
	}

	code->Release();

	hr = D3DCompileFromFile(
		L"../../Media/ShadersDX11/blinnphong11.hlsl",
		NULL, NULL, "ps_main", "ps_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();

		return false;
	}

	if (FAILED(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &pixelshader))) {
		MYERROR("Could not create pixel shader");
		return false;
	}

	code->Release();

	// create uniform buffer
	D3D11_BUFFER_DESC buffdesc;

	buffdesc.BindFlags				= D3D11_BIND_CONSTANT_BUFFER;
	buffdesc.Usage					= D3D11_USAGE_DEFAULT;
	buffdesc.ByteWidth				= sizeof(UniformData);
	buffdesc.CPUAccessFlags			= 0;
	buffdesc.StructureByteStride	= 0;
	buffdesc.MiscFlags				= 0;

	if (FAILED(device->CreateBuffer(&buffdesc, NULL, &uniformbuffer))) {
		MYERROR("Could not create uniform buffer");
		return false;
	}

	// create color buffers
	D3D11_TEXTURE2D_DESC texdesc;

	texdesc.ArraySize			= 1;
	texdesc.BindFlags			= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags		= 0;
	texdesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
	texdesc.Width				= screenwidth;
	texdesc.Height				= screenheight;
	texdesc.MipLevels			= 1;
	texdesc.SampleDesc.Count	= 1;
	texdesc.SampleDesc.Quality	= 0;
	texdesc.Usage				= D3D11_USAGE_DEFAULT;
	texdesc.MiscFlags			= 0;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &bottomlayercolor))) {
		MYERROR("Could not create color buffer 1");
		return false;
	}

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &feedbacklayercolor))) {
		MYERROR("Could not create color buffer 2");
		return false;
	}

	// create shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;

	srvdesc.Format						= texdesc.Format;
	srvdesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MipLevels			= 1;
	srvdesc.Texture2D.MostDetailedMip	= 0;

	if (FAILED(device->CreateShaderResourceView(bottomlayercolor, &srvdesc, &bottomlayersrv))) {
		MYERROR("Could not create shader resource view 1");
		return false;
	}

	if (FAILED(device->CreateShaderResourceView(feedbacklayercolor, &srvdesc, &feedbacklayersrv))) {
		MYERROR("Could not create shader resource view 2");
		return false;
	}

	// create depth buffers
	texdesc.BindFlags			= D3D11_BIND_DEPTH_STENCIL;
	texdesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &bottomlayerdepth))) {
		MYERROR("Could not create depth buffer 1");
		return false;
	}

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &feedbacklayerdepth))) {
		MYERROR("Could not create depth buffer 2");
		return false;
	}

	// create render target views
	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;

	rtvdesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
	rtvdesc.ViewDimension		= D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice	= 0;

	if (FAILED(device->CreateRenderTargetView(bottomlayercolor, &rtvdesc, &bottomlayerrtv))) {
		MYERROR("Could not create render target view 1");
		return false;
	}

	if (FAILED(device->CreateRenderTargetView(feedbacklayercolor, &rtvdesc, &feedbacklayerrtv))) {
		MYERROR("Could not create render target view 2");
		return false;
	}

	// create depth-stennvil views
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
	
	dsvdesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvdesc.ViewDimension		= D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvdesc.Texture2D.MipSlice	= 0;
	dsvdesc.Flags				= 0;

	if (FAILED(device->CreateDepthStencilView(bottomlayerdepth, &dsvdesc, &bottomlayerdsv))) {
		MYERROR("Could not create depth-stencil view 1");
		return false;
	}

	if (FAILED(device->CreateDepthStencilView(feedbacklayerdepth, &dsvdesc, &feedbacklayerdsv))) {
		MYERROR("Could not create depth-stencil view 2");
		return false;
	}

	// create some texture
	D3D11_SUBRESOURCE_DATA initdata;
	UINT* pixels = NULL;

	texdesc.ArraySize			= 1;
	texdesc.BindFlags			= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags		= 0;
	texdesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
	texdesc.Width				= 128;
	texdesc.Height				= 128;
	texdesc.MipLevels			= 1;
	texdesc.Usage				= D3D11_USAGE_DEFAULT;
	texdesc.MiscFlags			= D3D11_RESOURCE_MISC_GENERATE_MIPS;

	pixels = new UINT[texdesc.Width * texdesc.Height];

	initdata.SysMemPitch		= texdesc.Width * 4;
	initdata.SysMemSlicePitch	= texdesc.Width * texdesc.Height * 4;
	initdata.pSysMem			= pixels;

	// lazy to write compute shader
	Math::Color color;

	for (UINT blockx = 0; blockx < texdesc.Width / 16; ++blockx) {
		for (UINT blocky = 0; blocky < texdesc.Height / 16; ++blocky) {
			for (UINT threadx = 0; threadx < 16; ++threadx) {
				for (UINT thready = 0; thready < 16; ++thready) {
					UINT locx = blockx * 16 + threadx;
					UINT locy = blocky * 16 + thready;

					color = 0xff000000;

					if ((locx / 16 + locy / 16) % 2 == 1)
						color = 0xffffffff;
	
					pixels[locy * texdesc.Height + locx] = color;
				}
			}
		}
	}

	if (FAILED(device->CreateTexture2D(&texdesc, &initdata, &texture))) {
		MYERROR("Could not create texture");
		return false;
	}

	delete[] pixels;

	if (FAILED(device->CreateShaderResourceView(texture, &srvdesc, &texturesrv))) {
		MYERROR("Could not create shader resource view");
		return false;
	}

	context->GenerateMips(texturesrv);

	// create sampler state
	D3D11_SAMPLER_DESC sampdesc;

	sampdesc.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.ComparisonFunc	= D3D11_COMPARISON_NEVER;
	sampdesc.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampdesc.MaxAnisotropy	= 1;
	sampdesc.MaxLOD			= FLT_MAX;
	sampdesc.MinLOD			= -FLT_MAX;
	sampdesc.MipLODBias		= 0;

	sampdesc.BorderColor[0]	= 1;
	sampdesc.BorderColor[1]	= 1;
	sampdesc.BorderColor[2]	= 1;
	sampdesc.BorderColor[3]	= 1;

	if (FAILED(device->CreateSamplerState(&sampdesc, &linearsampler))) {
		MYERROR("Could not create sampler state");
		return false;
	}

	// create rasterizer state
	D3D11_RASTERIZER_DESC rasterdesc;

	rasterdesc.AntialiasedLineEnable	= FALSE;
	rasterdesc.CullMode					= D3D11_CULL_BACK;
	rasterdesc.DepthBias				= 0;
	rasterdesc.DepthBiasClamp			= 0;
	rasterdesc.DepthClipEnable			= TRUE;
	rasterdesc.FillMode					= D3D11_FILL_SOLID;
	rasterdesc.FrontCounterClockwise	= TRUE;
	rasterdesc.MultisampleEnable		= FALSE;
	rasterdesc.ScissorEnable			= FALSE;
	rasterdesc.SlopeScaledDepthBias		= 0;

	if (FAILED(device->CreateRasterizerState(&rasterdesc, &rasterstate))) {
		MYERROR("Could not create rasterizer state");
		return false;
	}

	// other
	screenquad = new D3D11ScreenQuad(device, L"../../Media/ShadersDX11/combinelayers11.hlsl");

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
	
	SAFE_RELEASE(linearsampler);
	SAFE_RELEASE(texturesrv);
	SAFE_RELEASE(texture);
	SAFE_RELEASE(uniformbuffer);
	SAFE_RELEASE(rasterstate);
	SAFE_RELEASE(vertexshader);
	SAFE_RELEASE(pixelshader);

	SAFE_RELEASE(feedbacklayersrv);
	SAFE_RELEASE(bottomlayersrv);
	SAFE_RELEASE(feedbacklayerdsv);
	SAFE_RELEASE(bottomlayerdsv);
	SAFE_RELEASE(feedbacklayerrtv);
	SAFE_RELEASE(bottomlayerrtv);
	SAFE_RELEASE(defaultview);

	SAFE_RELEASE(feedbacklayerdepth);
	SAFE_RELEASE(bottomlayerdepth);
	SAFE_RELEASE(feedbacklayercolor);
	SAFE_RELEASE(bottomlayercolor);
}

void KeyUp(KeyCode key)
{
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonMiddle) {
		// TODO:
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	UniformData		uniforms;
	Math::Matrix	view, proj;
	Math::Matrix	tmp;
	float			clearcolor[4] = { 0.0f, 0.0103f, 0.0707f, 1.0f };
	float			transparent[4] = { 0, 0, 0, 0 };

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)uniforms.eye);

	Math::MatrixMultiply(uniforms.viewproj, view, proj);
	Math::MatrixIdentity(uniforms.world);

	uniforms.world._41 = -0.108f;
	uniforms.world._42 = -0.7875f;
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 1, 0, 0);
	Math::MatrixMultiply(uniforms.world, uniforms.world, tmp);
	
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(time * 20.0f, 360.0f)), 0, 1, 0);
	Math::MatrixMultiply(uniforms.world, uniforms.world, tmp);

	// global uniforms
	uniforms.lightpos		= { -10, 8, 5, 1 };
	uniforms.uvscale		= { 1, 1 };

	// zero out resources
	ID3D11ShaderResourceView* nullsrvs[] = { NULL, NULL };
	context->PSSetShaderResources(0, 2, nullsrvs);

	// general state
	context->VSSetShader(vertexshader, NULL, 0);
	context->PSSetShader(pixelshader, NULL, 0);
	context->RSSetState(rasterstate);

	// render first teapot to bottom layer
	context->OMSetRenderTargets (1, &bottomlayerrtv, bottomlayerdsv);
	context->ClearRenderTargetView (bottomlayerrtv, clearcolor);
	context->ClearDepthStencilView(bottomlayerdsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);

	uniforms.lightcolor		= { 0, 1, 0, 1 };
	uniforms.lightambient	= { 0, 0.01f, 0, 1 };
	uniforms.world._41		-= 0.75f;

	Math::MatrixInverse(uniforms.worldinv, uniforms.world);
	context->UpdateSubresource(uniformbuffer, 0, NULL, &uniforms, 0, 0);

	context->VSSetConstantBuffers(1, 1, &uniformbuffer);
	context->PSSetConstantBuffers(1, 1, &uniformbuffer);
	context->PSSetShaderResources(0, 1, &texturesrv);
	context->PSSetSamplers(0, 1, &linearsampler);
	
	mesh->Draw(context);

	// copy depth from bottom layer to feedback layer
	context->CopyResource(feedbacklayerdepth, bottomlayerdepth);

	// render second teapot to feedback layer
	context->OMSetRenderTargets (1, &feedbacklayerrtv, feedbacklayerdsv);
	context->ClearRenderTargetView (feedbacklayerrtv, transparent);

	uniforms.lightcolor		= { 1, 0.5f, 0, 1 };
	uniforms.lightambient	= { 0.01f, 0.005f, 0, 1 };
	uniforms.world._41		+= 1.5f;

	Math::MatrixInverse(uniforms.worldinv, uniforms.world);
	context->UpdateSubresource(uniformbuffer, 0, NULL, &uniforms, 0, 0);

	//context->VSSetConstantBuffers(1, 1, &uniformbuffer);
	//context->PSSetConstantBuffers(1, 1, &uniformbuffer);

	mesh->Draw(context);

	// combine layers
	ID3D11RenderTargetView* rtvs[] = {
		defaultview,
		NULL
	};

	ID3D11ShaderResourceView* srvs[] = {
		bottomlayersrv,
		feedbacklayersrv
	};

	context->OMSetRenderTargets (2, rtvs, NULL);
	context->ClearRenderTargetView (defaultview, clearcolor);
	context->PSSetShaderResources(0, 2, srvs);

	screenquad->Draw(context);

	// present
	time += elapsedtime;
	app->Present();
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIDirect3D11)) {
		delete app;
		return 1;
	}

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
