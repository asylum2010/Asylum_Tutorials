
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

#include <Windows.h>
#include <wincodec.h>

#include "..\Common\application.h"
#include "..\Common\d2dext.h"
#include "..\Common\win32openfiledialog.h"
#include "..\Common\basiccamera.h"

#define TEXT_EMSIZE			25.0f

// helper macros
#define TITLE				"Sample 73: Direct2D symbol fill"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*				app				= nullptr;
BasicCamera					camera;
D3D11InstancedRenderer*		d3d11renderer	= nullptr;
D3D11LinearGradientBrush*	d3d11gradient	= nullptr;

ID3D11DeviceContext*		context			= nullptr;
ID3D11RenderTargetView*		defaultview		= nullptr;
ID3D11RenderTargetView*		offscreenview	= nullptr;

ID2D1Factory*				d2dfactory		= nullptr;
ID2D1RenderTarget*			d2drendertarget	= nullptr;
ID2D1Bitmap*				sharedbitmap	= nullptr;

ID2D1PathGeometry*			spindel			= nullptr;
ID2D1PathGeometry*			star			= nullptr;
ID2D1SolidColorBrush*		blackbrush		= nullptr;
ID2D1LinearGradientBrush*	gradientbrush	= nullptr;

IDWriteFactory*				dwritefactory	= nullptr;
IDWriteTextFormat*			textformat		= nullptr;
IDWriteTextLayout*			textlayout		= nullptr;
OutlineTextRenderer*		textrenderer	= nullptr;

bool InitScene()
{
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE))) {
		std::cout << "COM initialization failed!\n";
		return false;
	}

	context = (ID3D11DeviceContext*)app->GetDeviceContext();

	uint32_t			screenwidth		= app->GetClientWidth();
	uint32_t			screenheight	= app->GetClientHeight();
	ID3D11Device*		device			= (ID3D11Device*)app->GetLogicalDevice();
	IDXGISwapChain*		swapchain		= (IDXGISwapChain*)app->GetSwapChain();
	ID3D11Texture2D*	backbuffer		= nullptr;
	IDXGISurface*		surface			= nullptr;

	if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer)))
		return false;

	if (FAILED(device->CreateRenderTargetView(backbuffer, NULL, &defaultview))) {
		backbuffer->Release();
		return false;
	}

	if (FAILED(backbuffer->QueryInterface(__uuidof(IDXGISurface), (void**)&surface))) {
		backbuffer->Release();
		return false;
	}

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dfactory))) {
		MessageBox(NULL, "Could not create Direct2D factory", "Fatal error", MB_OK);
		return false;
	}

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();

	props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);

	if (FAILED(d2dfactory->CreateDxgiSurfaceRenderTarget(surface, props, &d2drendertarget))) {
		MessageBox(NULL, "Could not create Direct2D render target", "Fatal error", MB_OK);
		return false;
	}

	surface->Release();
	backbuffer->Release();

	// create shared render target
	D3D11_TEXTURE2D_DESC texdesc;
	D2D1_BITMAP_PROPERTIES bitmapprops;
	ID3D11Texture2D* offscreentarget = nullptr;
	IDXGISurface* offscreensurface = nullptr;

	texdesc.ArraySize			= 1;
	texdesc.BindFlags			= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags		= 0;
	texdesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	texdesc.Height				= screenheight;
	texdesc.MipLevels			= 1;
	texdesc.MiscFlags			= 0;
	texdesc.SampleDesc.Count	= 1;
	texdesc.SampleDesc.Quality	= 0;
	texdesc.Usage				= D3D11_USAGE_DEFAULT;
	texdesc.Width				= screenwidth;

	if (FAILED(device->CreateTexture2D(&texdesc, NULL, &offscreentarget))) {
		std::cout << "Could not create offscreen target!\n";
		return false;
	}

	if (FAILED(device->CreateRenderTargetView(offscreentarget, NULL, &offscreenview))) {
		std::cout << "Could not create offscreen view!\n";
		return false;
	}

	if (FAILED(offscreentarget->QueryInterface(__uuidof(IDXGISurface), (void**)&offscreensurface))) {
		std::cout << "Texture is not an IDXGISurface!\n";
		return false;
	}

	bitmapprops.dpiX = 96;
	bitmapprops.dpiY = 96;
	bitmapprops.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);

	if (FAILED(d2drendertarget->CreateSharedBitmap(__uuidof(IDXGISurface), offscreensurface, &bitmapprops, &sharedbitmap))) {
		std::cout << "Could not create shared bitmap!\n";
		return false;
	}

	offscreensurface->Release();
	offscreentarget->Release();

	// create geometry
	ID2D1GeometrySink* sink = nullptr;

	d2dfactory->CreatePathGeometry(&spindel);

	spindel->Open(&sink);
	{
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(D2D1::Point2F(0, 0), D2D1_FIGURE_BEGIN_FILLED);

		sink->AddLine(D2D1::Point2F(200, 0));

		sink->AddBezier(D2D1::BezierSegment(
			D2D1::Point2F(150, 50),
			D2D1::Point2F(150, 150),
			D2D1::Point2F(200, 200)));

		sink->AddLine(D2D1::Point2F(0, 200));

		sink->AddBezier(D2D1::BezierSegment(
			D2D1::Point2F(50, 150),
			D2D1::Point2F(50, 50),
			D2D1::Point2F(0, 0)));

		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	}
	sink->Close();
	sink->Release();

	// create D3D11 representation
	d3d11renderer = new D3D11InstancedRenderer(device, context);

	d3d11renderer->SetGeometry(spindel, 10);

	D2D1_SIZE_F			size			= d2drendertarget->GetSize();
	D2D1_RECT_F			bounds;
	float				spacing			= 10;

	spindel->GetBounds(D2D1::Matrix3x2F::Identity(), &bounds);

	D2D1_POINT_2F		center = { 0.5f * (bounds.right - bounds.left), 0.5f * (bounds.bottom - bounds.top) };
	D2D1::Matrix3x2F	transform = D2D1::Matrix3x2F::Rotation(-30, center) * D2D1::Matrix3x2F::Scale(0.2f, 0.2f);

	spindel->GetBounds(transform, &bounds);

	D2D1_SIZE_F			sizewithspacing	= { bounds.right - bounds.left + spacing, bounds.bottom - bounds.top + spacing };
	int					columns			= (int)(size.width / sizewithspacing.width);
	int					rows			= (int)(size.height / sizewithspacing.height);
	
	// generate instance data
	bool success = d3d11renderer->SetInstanceData(columns * rows, [&](UINT index, Math::Matrix& insttraf) -> void {
		UINT i = index / rows;
		UINT j = index % rows;

		D2D1::Matrix3x2F offset = D2D1::Matrix3x2F::Translation(spacing + i * sizewithspacing.width, spacing + j * sizewithspacing.height);
		D2D1::Matrix3x2F combined = transform * offset;
		
		insttraf._11 = combined._11;	insttraf._12 = combined._21;	insttraf._13 = 0;	insttraf._14 = combined._31;
		insttraf._21 = combined._12;	insttraf._22 = combined._22;	insttraf._23 = 0;	insttraf._24 = combined._32;
		insttraf._31 = 0;				insttraf._32 = 0;				insttraf._33 = 1;	insttraf._34 = 0;
		insttraf._41 = 0;				insttraf._42 = 0;				insttraf._43 = 0;	insttraf._44 = 1;
	});

	if (!success) {
		std::cout << "Could not set instance data!\n";
		return false;
	}

	// create clip geometry
	const uint32_t segments = 16;
	D2D1_POINT_2F pt;

	d2dfactory->CreatePathGeometry(&star);

	star->Open(&sink);
	{
		auto _starvertex = [=](D2D1_POINT_2F& out, uint32_t i) -> void {
			if (i % 2 == 0) {
				out.x = sinf((Math::TWO_PI / segments) * i) * 300;
				out.y = cosf((Math::TWO_PI / segments) * i) * 300;
			} else {
				out.x = sinf((Math::TWO_PI / segments) * i) * 120;
				out.y = cosf((Math::TWO_PI / segments) * i) * 120;
			}
		};

		_starvertex(pt, 0);

		sink->BeginFigure(pt, D2D1_FIGURE_BEGIN_FILLED);

		for (uint32_t i = 1; i < segments; ++i) {
			_starvertex(pt, i);
			sink->AddLine(pt);
		}

		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	}
	sink->Close();
	sink->Release();

	d2drendertarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &blackbrush);
	
	ID2D1GradientStopCollection* gradientstops = nullptr;

	const D2D1_GRADIENT_STOP stops[] = {
		{ 0, { 0, 1, 1, 0.25f } },
		{ 1, { 0, 0, 1, 1 } }
	};

	d2drendertarget->CreateGradientStopCollection(stops, ARRAY_SIZE(stops), &gradientstops);

	auto gradientprops = D2D1::LinearGradientBrushProperties(D2D1::Point2F(100, 0), D2D1::Point2F(100, 200));

	d2drendertarget->CreateLinearGradientBrush(
		gradientprops,
		D2D1::BrushProperties(),
		gradientstops,
		&gradientbrush);

	gradientstops->Release();

	d3d11gradient = new D3D11LinearGradientBrush(device, context, gradientprops, stops, ARRAY_SIZE(stops));

	// create DirectWrite objects
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwritefactory)))) {
		std::cout << "DirectWrite initialization failed!\n";
		return false;
	}

	if (FAILED(dwritefactory->CreateTextFormat(
		L"Arial", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL, 72.0f, L"en-us", &textformat)))
	{
		std::cout << "Could not create text format!\n";
		return false;
	}

	std::wstring text(L"// TODO:");

	if (FAILED(dwritefactory->CreateTextLayout(
		text.c_str(), (UINT)text.length(), textformat, 512.0f, 512.0f, &textlayout)))
	{
		std::cout << "Could not create text layout!\n";
		return false;
	}
	
	DWRITE_TEXT_RANGE range = { 0, (UINT32)text.length() };

	textlayout->SetFontSize(TEXT_EMSIZE, range);
	textlayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
	
	textrenderer = new OutlineTextRenderer(d2dfactory, d2drendertarget);

	return true;
}

void UninitScene()
{
	delete textrenderer;
	delete d3d11gradient;

	SAFE_RELEASE(d3d11renderer);

	SAFE_RELEASE(gradientbrush);
	SAFE_RELEASE(blackbrush);
	SAFE_RELEASE(star);
	SAFE_RELEASE(spindel);

	SAFE_RELEASE(textlayout);
	SAFE_RELEASE(textformat);
	SAFE_RELEASE(dwritefactory);

	SAFE_RELEASE(sharedbitmap);
	SAFE_RELEASE(d2drendertarget);
	SAFE_RELEASE(d2dfactory);

	SAFE_RELEASE(offscreenview);
	SAFE_RELEASE(defaultview);

	CoUninitialize();
}

void KeyUp(KeyCode key)
{
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonMiddle) {
		camera.PanRight(dx * -1.0f);
		camera.PanUp(dy * -1.0f);
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
	D2D1::Matrix3x2F	transform;
	Math::Vector3		eye;

	camera.Animate(alpha);
	camera.GetEyePosition(eye);

	// render D3D content to offscreen texture
	context->OMSetRenderTargets(1, &offscreenview, NULL);
	d3d11renderer->Draw();

	// render D2D content
	d2drendertarget->BeginDraw();
	d2drendertarget->Clear(D2D1::ColorF(0.0f, 0.0103f, 0.0707f, 1.0f));
	{
		D2D1_SIZE_F size = d2drendertarget->GetSize();

		// draw clip geometry first
		d2drendertarget->SetTransform(D2D1::Matrix3x2F::Translation(size.width * 0.5f, size.height * 0.5f));
		d2drendertarget->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), star), NULL);

		d2drendertarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		d2drendertarget->SetTransform(D2D1::Matrix3x2F::Identity());
		d2drendertarget->DrawBitmap(sharedbitmap, NULL, 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

#if TEST_RENDER_D2D
		// then symbol fill
		D2D1_RECT_F			bounds;
		float				spacing			= 10;

		spindel->GetBounds(D2D1::Matrix3x2F::Identity(), &bounds);

		D2D1_POINT_2F		center = { 0.5f * (bounds.right - bounds.left), 0.5f * (bounds.bottom - bounds.top) };
		D2D1::Matrix3x2F	transform = D2D1::Matrix3x2F::Rotation(-30, center) * D2D1::Matrix3x2F::Scale(0.2f, 0.2f);

		spindel->GetBounds(transform, &bounds);

		D2D1_SIZE_F			sizewithspacing	= { bounds.right - bounds.left + spacing, bounds.bottom - bounds.top + spacing };
		int					columns			= (int)(size.width / sizewithspacing.width);
		int					rows			= (int)(size.height / sizewithspacing.height);

		for (int i = 0; i < columns; ++i) {
			for (int j = 0; j < rows; ++j) {

				D2D1::Matrix3x2F offset = D2D1::Matrix3x2F::Translation(spacing + i * sizewithspacing.width, spacing + j * sizewithspacing.height);
				d2drendertarget->SetTransform(transform * offset);

				d2drendertarget->DrawGeometry(spindel, blackbrush, 10);
				d2drendertarget->FillGeometry(spindel, gradientbrush);
			}
		}
#endif

		// pop and continue
		d2drendertarget->PopLayer();

		d2drendertarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		d2drendertarget->SetTransform(D2D1::Matrix3x2F::Translation(size.width * 0.5f, size.height * 0.5f));
		d2drendertarget->DrawGeometry(star, blackbrush, 10);

		// draw help text
		d2drendertarget->SetTransform(D2D1::Matrix3x2F::Identity());
		textlayout->Draw(NULL, textrenderer, 10.0f, 10.0f);
	}
	d2drendertarget->EndDraw();

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
