
#pragma comment(lib, "d2d1.lib")
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

#define REFERENCE_DISTANCE	2.0f
#define TILE_SIZE			1024
#define TEXT_EMSIZE			25.0f

// helper macros
#define TITLE				"Sample 73: Direct2D images"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*				app				= nullptr;
BasicCamera					camera;
UINT						imgwidth		= 0;
UINT						imgheight		= 0;

std::vector<ID2D1Bitmap*>	tiles;
ID2D1HwndRenderTarget*		rendertarget	= nullptr;

IWICImagingFactory*			imagefactory	= nullptr;
IWICFormatConverter*		convertedbitmap	= nullptr;

IDWriteFactory*				dwritefactory	= nullptr;
IDWriteTextFormat*			textformat		= nullptr;
IDWriteTextLayout*			textlayout		= nullptr;
OutlineTextRenderer*		textrenderer	= nullptr;

static void CalculateCameraDistances(float& defaultdist, float& mindist, float& maxdist)
{
	// NOTE: distance can be defined arbirarily wrt. an ortographic camera as long as it's linear

	D2D1_SIZE_F imgsize = { (FLOAT)imgwidth, (FLOAT)imgheight };
	D2D1_SIZE_F rtsize = rendertarget->GetSize();

	float ratio1 = rtsize.width / imgsize.width;
	float ratio2 = rtsize.height / imgsize.height;
	float scale = std::min<float>(ratio1, ratio2);

	scale = std::min<float>(2.0f, scale);

	defaultdist = REFERENCE_DISTANCE / scale;
	mindist = REFERENCE_DISTANCE / 2.0f;
	maxdist = defaultdist * 1.5f;
}

static bool WICLoadImage(LPCWSTR filename)
{
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	HRESULT hr = S_OK;

	if (FAILED(imagefactory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder))) {
		std::cout << "WICLoadImage(): Could not create decoder!\n";
		return false;
	}

	if (FAILED(decoder->GetFrame(0, &frame))) {
		decoder->Release();
		return false;
	}

	if (convertedbitmap)
		convertedbitmap->Release();

	imagefactory->CreateFormatConverter(&convertedbitmap);
	convertedbitmap->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);

	for (ID2D1Bitmap* d2dbitmap : tiles) {
		if (d2dbitmap)
			d2dbitmap->Release();
	}

	tiles.clear();

	convertedbitmap->GetSize(&imgwidth, &imgheight);

	if (imgwidth > TILE_SIZE || imgheight > TILE_SIZE) {
		// tile up
		D2D1_BITMAP_PROPERTIES props;
		WICRect rect;
		BYTE* stagingbuffer = new BYTE[TILE_SIZE * TILE_SIZE * sizeof(UINT32)];
		ID2D1Bitmap* d2dbitmap = nullptr;

		UINT remainingwidth = imgwidth;
		UINT remainingheight = imgheight;

		// NOTE: https://docs.microsoft.com/en-us/windows/win32/direct2d/supported-pixel-formats-and-alpha-modes#supported-formats-for-wic-bitmap-render-target

		props.dpiX = 96;
		props.dpiY = 96;
		props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

		for (UINT i = 0; i < imgwidth; i += TILE_SIZE) {
			remainingheight = imgheight;

			for (UINT j = 0; j < imgheight; j += TILE_SIZE) {
				rect.X = i;
				rect.Y = j;
				rect.Width = std::min<UINT>(TILE_SIZE, remainingwidth);
				rect.Height = std::min<UINT>(TILE_SIZE, remainingheight);

				hr = convertedbitmap->CopyPixels(&rect, rect.Width * sizeof(UINT32), TILE_SIZE * TILE_SIZE * sizeof(UINT32), stagingbuffer);

				if (FAILED(hr)) {
					std::cout << "WICLoadImage(): CopyPixels() failed!\n";
					return false;
				}

				UINT32 pitch = rect.Width * sizeof(UINT32);
				hr = rendertarget->CreateBitmap(D2D1::SizeU(rect.Width, rect.Height), stagingbuffer, pitch, &props, &d2dbitmap);

				if (FAILED(hr)) {
					std::cout << "WICLoadImage(): CreateBitmap() failed!\n";
					return false;
				}

				tiles.push_back(d2dbitmap);
				remainingheight -= rect.Height;
			}

			remainingwidth -= rect.Width;
		}

		delete[] stagingbuffer;
	} else {
		tiles.push_back(nullptr);
		hr = rendertarget->CreateBitmapFromWicBitmap(convertedbitmap, NULL, &tiles[0]);
	}

	frame->Release();
	decoder->Release();

	float defaultdist;
	float mindist, maxdist;

	CalculateCameraDistances(defaultdist, mindist, maxdist);

	camera.SetPosition(0, 0, 0);
	camera.SetDistance(defaultdist);
	camera.SetZoomLimits(mindist, maxdist);
	camera.SetOrientation(0, 0, 0);

	return SUCCEEDED(hr);
}

bool InitScene()
{
	rendertarget = (ID2D1HwndRenderTarget*)app->GetSwapChain();

	UINT32 maxsize = rendertarget->GetMaximumBitmapSize();
	std::cout << "Maximum bitmap size supported by device: " << maxsize << std::endl;

	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE))) {
		std::cout << "COM initialization failed!\n";
		return false;
	}

	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imagefactory)))) {
		std::cout << "Could not create image factory!\n";
		return false;
	}

	// load default image
	WICLoadImage(L"../../Media/Textures/scarlett2.jpg");

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

	std::wstring text(L"Middle mouse - Pan/zoom\n\nPress O to browse for picture");

	if (FAILED(dwritefactory->CreateTextLayout(
		text.c_str(), (UINT)text.length(), textformat, 512.0f, 512.0f, &textlayout)))
	{
		std::cout << "Could not create text layout!\n";
		return false;
	}
	
	DWRITE_TEXT_RANGE range = { 0, (UINT32)text.length() };

	textlayout->SetFontSize(TEXT_EMSIZE, range);
	textlayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
	
	textrenderer = new OutlineTextRenderer((ID2D1Factory*)app->GetDriverInterface(), rendertarget);

	return true;
}

void UninitScene()
{
	delete textrenderer;

	for (ID2D1Bitmap* d2dbitmap : tiles) {
		if (d2dbitmap)
			d2dbitmap->Release();
	}

	SAFE_RELEASE(textlayout);
	SAFE_RELEASE(textformat);
	SAFE_RELEASE(convertedbitmap);
	SAFE_RELEASE(dwritefactory);
	SAFE_RELEASE(imagefactory);

	CoUninitialize();
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCodeO: {
		Win32OpenFileDialog openfile;

		openfile.AddFilter("Image Files", "@jpg;png;bmp");

		DialogResult result = openfile.Show(app->GetHandle());

		if (result == DialogResultOk) {
			std::wstring wstr;
			int length = MultiByteToWideChar(CP_UTF8, 0, &openfile.FileNames[0][0], (int)openfile.FileNames[0].length(), NULL, 0);

			wstr.resize(length, '\0');
			MultiByteToWideChar(CP_UTF8, 0, &openfile.FileNames[0][0], (int)openfile.FileNames[0].length(), &wstr[0], length);

			WICLoadImage(wstr.c_str());
		}
	} break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / REFERENCE_DISTANCE;

		camera.PanRight(dx * -scale);
		camera.PanUp(dy * -scale);
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

	rendertarget->BeginDraw();
	rendertarget->Clear(D2D1::ColorF(0.0f, 0.125f, 0.3f, 1.0f));
	{
		D2D1_SIZE_F imgsize = { (FLOAT)imgwidth, (FLOAT)imgheight };
		D2D1_SIZE_F rtsize = rendertarget->GetSize();
		size_t counter = 0;

		float x = (rtsize.width - imgsize.width) * 0.5f - eye.x;
		float y = (rtsize.height - imgsize.height) * 0.5f - eye.y;
		float scale = REFERENCE_DISTANCE / camera.GetDistance();

		D2D1_POINT_2F scalingcenter = D2D1::Point2F(imgsize.width * 0.5f + eye.x, imgsize.height * 0.5f + eye.y);

		transform = D2D1::Matrix3x2F::Scale(scale, scale, scalingcenter);
		transform = transform * D2D1::Matrix3x2F::Translation(x, y);

		rendertarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

		for (UINT i = 0; i < imgwidth; i += TILE_SIZE) {
			for (UINT j = 0; j < imgheight; j += TILE_SIZE) {
				rendertarget->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)i, (FLOAT)j) * transform);
				rendertarget->DrawBitmap(tiles[counter], NULL);

				++counter;
			}
		}

		// draw help text
		rendertarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		rendertarget->SetTransform(D2D1::Matrix3x2F::Identity());

		textlayout->Draw(NULL, textrenderer, 10.0f, 10.0f);
	}
	rendertarget->EndDraw();
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIDirect2D)) {
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
