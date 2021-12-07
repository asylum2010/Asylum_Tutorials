
#include "win32application.h"

#ifdef OPENGL
#	include "glextensions.h"
#endif

#include <iostream>
#include <gdiplus.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#	define ENABLE_VALIDATION
#endif

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof(x[0]))
#define V_RETURN(r, e, x)	{ if (!(x)) { std::cerr << "* Error: " << e << "!\n"; return r; }}

#ifdef _DEBUG
static _CrtMemState _memstate;
#endif

static Win32Application* _app = nullptr;

static LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	if (hWnd != _app->hwnd)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		if (_app->KeyDownCallback)
			_app->KeyDownCallback((KeyCode)wParam);

		break;

	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			if (hWnd == _app->hwnd)
				SendMessage(hWnd, WM_CLOSE, 0, 0);

			break;

		default:
			if (_app->KeyUpCallback)
				_app->KeyUpCallback((KeyCode)wParam);

			break;
		} break;

	case WM_MOUSEMOVE: {
		short x = (short)(lParam & 0xffff);
		short y = (short)((lParam >> 16) & 0xffff);

		_app->inputstate.dX = x - _app->inputstate.X;
		_app->inputstate.dY = y - _app->inputstate.Y;

		_app->inputstate.X = x;
		_app->inputstate.Y = y;

		if (_app->MouseMoveCallback != nullptr)
			_app->MouseMoveCallback(_app->inputstate.X, _app->inputstate.Y, _app->inputstate.dX, _app->inputstate.dY);
		} break;

	case WM_MOUSEWHEEL: {
		short x = (short)(lParam & 0xffff);
		short y = (short)((lParam >> 16) & 0xffff);
		short delta = (short)((wParam >> 16) & 0xffff);

		_app->inputstate.dZ = delta / WHEEL_DELTA;

		if (_app->MouseScrollCallback != nullptr)
			_app->MouseScrollCallback(x, y, _app->inputstate.dZ);
		} break;

	case WM_LBUTTONDOWN:
		_app->inputstate.Button |= MouseButtonLeft;

		if (_app->MouseDownCallback)
			_app->MouseDownCallback(MouseButtonLeft, _app->inputstate.X, _app->inputstate.Y);

		break;

	case WM_RBUTTONDOWN:
		_app->inputstate.Button |= MouseButtonRight;

		if (_app->MouseDownCallback)
			_app->MouseDownCallback(MouseButtonRight, _app->inputstate.X, _app->inputstate.Y);

		break;

	case WM_MBUTTONDOWN:
		_app->inputstate.Button |= MouseButtonMiddle;

		if (_app->MouseDownCallback)
			_app->MouseDownCallback(MouseButtonMiddle, _app->inputstate.X, _app->inputstate.Y);

		break;

	case WM_LBUTTONUP:
		if (_app->MouseUpCallback && (_app->inputstate.Button & MouseButtonLeft))
			_app->MouseUpCallback(MouseButtonLeft, _app->inputstate.X, _app->inputstate.Y);

		_app->inputstate.Button &= (~MouseButtonLeft);
		break;

	case WM_RBUTTONUP:
		if (_app->MouseUpCallback && (_app->inputstate.Button & MouseButtonRight))
			_app->MouseUpCallback(MouseButtonRight, _app->inputstate.X, _app->inputstate.Y);

		_app->inputstate.Button &= (~MouseButtonRight);
		break;

	case WM_MBUTTONUP:
		if (_app->MouseUpCallback && (_app->inputstate.Button & MouseButtonMiddle))
			_app->MouseUpCallback(MouseButtonMiddle, _app->inputstate.X, _app->inputstate.Y);

		_app->inputstate.Button &= (~MouseButtonMiddle);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifdef OPENGL
static void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
	if (type >= GL_DEBUG_TYPE_ERROR && type <= GL_DEBUG_TYPE_PERFORMANCE) {
		if (source == GL_DEBUG_SOURCE_API)
			std::cout << "GL(" << severity << "): ";
		else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER)
			std::cout << "GLSL(" << severity << "): ";
		else
			std::cout << "OTHER(" << severity << "): ";

		std::cout << id << ": " << message << "\n";
	}

	// bug in SwapBuffers() when Fraps is running
	if (id == 3200)
		return;

	if (type == GL_DEBUG_TYPE_ERROR)
		__debugbreak();
}
#endif

#ifdef VULKAN
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
	VkDebugReportFlagsEXT		flags,
	VkDebugReportObjectTypeEXT	objectType,
	uint64_t					object,
	size_t						location,
	int32_t						messageCode,
	const char*					pLayerPrefix,
	const char*					pMessage,
	void*						pUserData)
{
	// NOTE: validation layer bug
	//if (strcmp(pMessage, "Cannot clear attachment 1 with invalid first layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.") == 0)
	//	return VK_FALSE;

	std::cerr << pMessage << std::endl;
	__debugbreak();

	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsReportCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
	void*										pUserData)
{
	if (pCallbackData != nullptr)
		std::cerr << pCallbackData->pMessage << std::endl;

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		__debugbreak();
		return VK_FALSE;
	}

	return VK_TRUE;
}
#endif

Win32Application::Win32Application(uint32_t width, uint32_t height)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
	_CrtMemCheckpoint(&_memstate);
#endif

	_app	= this;

	hinst	= GetModuleHandle(NULL);
	hwnd	= nullptr;
	hdc		= nullptr;

#ifdef OPENGL
	hglrc = nullptr;
#endif

#ifdef DIRECT3D9
	d3dinterface = nullptr;
	device = nullptr;
#endif

#ifdef DIRECT3D10
	device = nullptr;
	swapchain = nullptr;
#endif

#ifdef DIRECT3D11
	device = nullptr;
	context = nullptr;
	swapchain = nullptr;
#endif

#ifdef DIRECT2D
	d2dfactory = nullptr;
	d2drendertarget = nullptr;
#endif

#ifdef VULKAN
	presenter = nullptr;
#endif

	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_OWNDC,
		(WNDPROC)WndProc,
		0L, 0L,
		hinst,
		NULL, LoadCursor(0, IDC_ARROW),
		NULL, NULL, "AsylumSampleClass", NULL
	};

	RegisterClassEx(&wc);

	gdiplustoken		= 0;
	clientwidth			= width;
	clientheight		= height;

	inputstate.Button	= 0;
	inputstate.X		= 0;
	inputstate.Y		= 0;
	inputstate.dX		= 0;
	inputstate.dY		= 0;

	Gdiplus::GdiplusStartupInput gdiplustartup;
	Gdiplus::GdiplusStartup(&gdiplustoken, &gdiplustartup, NULL);

	InitWindow();
}

Win32Application::~Win32Application()
{
	if (gdiplustoken != 0)
		Gdiplus::GdiplusShutdown(gdiplustoken);

	UnregisterClass("AsylumSampleClass", hinst);

#ifdef _DEBUG
	_CrtMemDumpAllObjectsSince(&_memstate);
	system("pause");
#endif
}

void Win32Application::InitWindow()
{
	RECT rect = { 0, 0, (LONG)clientwidth, (LONG)clientheight };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	// windowed mode
	style |= WS_SYSMENU|WS_BORDER|WS_CAPTION;
	AdjustWindow(rect, clientwidth, clientheight, style, 0);

	hwnd = CreateWindow("AsylumSampleClass", "Asylum's shader sample", style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, hinst, NULL);

	if (hwnd == nullptr) {
		MessageBox(NULL, "Could not create window", "Fatal error", MB_OK);
		exit(1);
	}
}

void Win32Application::AdjustWindow(RECT& out, uint32_t& width, uint32_t& height, DWORD style, DWORD exstyle, bool menu)
{
	RECT workarea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	uint32_t w = workarea.right - workarea.left;
	uint32_t h = workarea.bottom - workarea.top;

	out.left = ((LONG)w - (LONG)width) / 2;
	out.top = ((LONG)h - (LONG)height) / 2;
	out.right = (w + width) / 2;
	out.bottom = (h + height) / 2;

	AdjustWindowRectEx(&out, style, menu, 0);

	uint32_t windowwidth = out.right - out.left;
	uint32_t windowheight = out.bottom - out.top;

	uint32_t dw = windowwidth - width;
	uint32_t dh = windowheight - height;

	if (windowheight > h) {
		float ratio = (float)width / (float)height;
		float realw = (float)(h - dh) * ratio + 0.5f;

		windowheight = h;
		windowwidth = (uint32_t)floor(realw) + dw;
	}

	if (windowwidth > w) {
		float ratio = (float)height / (float)width;
		float realh = (float)(w - dw) * ratio + 0.5f;

		windowwidth = w;
		windowheight = (uint32_t)floor(realh) + dh;
	}

	out.left = workarea.left + (w - windowwidth) / 2;
	out.top = workarea.top + (h - windowheight) / 2;
	out.right = workarea.left + (w + windowwidth) / 2;
	out.bottom = workarea.top + (h + windowheight) / 2;

	width = windowwidth - dw;
	height = windowheight - dh;
}

bool Win32Application::InitializeOpenGL(bool core)
{
#ifdef OPENGL
	HWND windowhandle = hwnd;
	HWND dummy = nullptr;

	auto Cleanup = [&]() {
		if (hglrc != nullptr) {
			wglMakeCurrent(hdc, NULL);
			wglDeleteContext(hglrc);
		}

		ReleaseDC(windowhandle, hdc);
		hdc = nullptr;

		if (dummy != nullptr) {
			DestroyWindow(dummy);
			dummy = nullptr;
		}
	};

	if (core) {
		// create dummy window
		dummy = CreateWindow("AsylumSampleClass", "Dummy", WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			0, 0, 100, 100, 0, 0, GetModuleHandle(0), 0);

		windowhandle = dummy;
	}

	hdc = GetDC(windowhandle);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24, // color
		0, 0, 0, 0, 0, 0,
		0, // alpha
		0, 0, 0, 0, 0, 0,
		24, 8, 0, // depth, stencil, aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	int pixelformat = ChoosePixelFormat(hdc, &pfd);

	if (pixelformat == 0) {
		MessageBox(NULL, "Could not find suitable pixel format", "Fatal error", MB_OK);
		Cleanup();

		return false;
	}

	SetPixelFormat(hdc, pixelformat, &pfd);
	hglrc = wglCreateContext(hdc);

	if (hglrc == nullptr) {
		MessageBox(NULL, "Could not create OpenGL context", "Fatal error", MB_OK);
		Cleanup();

		return false;
	}

	wglMakeCurrent(hdc, hglrc);
	GLExtensions::QueryFeatures(hdc);

	if (core) {
		if (GLExtensions::GLVersion < GLExtensions::GL_3_3) {
			MessageBox(NULL, "Core profile not supported on this device", "Fatal error", MB_OK);
			Cleanup();

			return false;
		}

		if (!GLExtensions::WGL_ARB_pixel_format || !GLExtensions::WGL_ARB_create_context || !GLExtensions::WGL_ARB_create_context_profile) {
			MessageBox(NULL, "WGL_ARB_pixel_format not found", "Fatal error", MB_OK);
			Cleanup();

			return false;
		}

		// create core profile context
		int attrib[32];
		int i = 0;
		UINT numformats = 0;

		memset(attrib, 0, sizeof(attrib));

		attrib[i++] = 0x2001;		// WGL_DRAW_TO_WINDOW_ARB
		attrib[i++] = TRUE;
		attrib[i++] = 0x2003;		// WGL_ACCELERATION_ARB
		attrib[i++] = 0x2027;		// WGL_FULL_ACCELERATION_ARB
		attrib[i++] = 0x2010;		// WGL_SUPPORT_OPENGL_ARB
		attrib[i++] = TRUE;
		attrib[i++] = 0x2011;		// WGL_DOUBLE_BUFFER_ARB
		attrib[i++] = TRUE;
		attrib[i++] = 0x2013;		// WGL_PIXEL_TYPE_ARB
		attrib[i++] = 0x202B;		// WGL_TYPE_RGBA_ARB
		attrib[i++] = 0x2014;		// WGL_COLOR_BITS_ARB
		attrib[i++] = pfd.cColorBits = 32;
		attrib[i++] = 0x201B;		// WGL_ALPHA_BITS_ARB
		attrib[i++] = pfd.cAlphaBits = 0;
		attrib[i++] = 0x2022;		// WGL_DEPTH_BITS_ARB
		attrib[i++] = pfd.cDepthBits = 24;
		attrib[i++] = 0x2023;		// WGL_STENCIL_BITS_ARB
		attrib[i++] = pfd.cStencilBits = 8;
		attrib[i++] = 0;

		if (attrib[1])
			pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

		if (attrib[5])
			pfd.dwFlags |= PFD_SUPPORT_OPENGL;

		if (attrib[7])
			pfd.dwFlags |= PFD_DOUBLEBUFFER;

		if (attrib[9] == 0x2029)	// WGL_SWAP_COPY_ARB
			pfd.dwFlags |= PFD_SWAP_COPY;

		wglChoosePixelFormat(hdc, attrib, 0, 1, &pixelformat, &numformats);

		if (0 == numformats) {
			MessageBox(NULL, "wglChoosePixelFormat failed", "Fatal error", MB_OK);
			Cleanup();

			return false;
		}

		std::cout << "Selected pixel format: " << pixelformat <<"\n";

		wglMakeCurrent(hdc, NULL);
		wglDeleteContext(hglrc);

		DestroyWindow(dummy);
		dummy = nullptr;

		hdc = GetDC(hwnd);
		windowhandle = hwnd;
		hglrc = nullptr;

		int success = SetPixelFormat(hdc, pixelformat, &pfd);

		if (0 == success) {
			MessageBox(NULL, "Could not set pixel format", "Fatal error", MB_OK);
			Cleanup();

			return false;
		}

		int contextattribs[] = {
			0x2091,			// WGL_CONTEXT_MAJOR_VERSION_ARB
			GLExtensions::GetMajorVersion(),
			0x2092,			// WGL_CONTEXT_MINOR_VERSION_ARB
			GLExtensions::GetMinorVersion(),
			0x2094,			// WGL_CONTEXT_FLAGS_ARB
#ifdef _DEBUG
			0x0001,			// WGL_CONTEXT_DEBUG_BIT
#else
			0,
#endif
			0x9126,			// WGL_CONTEXT_PROFILE_MASK_ARB
			0x00000001,		// WGL_CONTEXT_CORE_PROFILE_BIT_ARB
			0
		};

		hglrc = wglCreateContextAttribs(hdc, NULL, contextattribs);

		if (hglrc == nullptr) {
			MessageBox(NULL, "Could not create OpenGL core profile context", "Fatal error", MB_OK);
			Cleanup();

			return false;
		}

		std::cout << "Created core profile context...\n";
		wglMakeCurrent(hdc, hglrc);
	}

	if (GLExtensions::WGL_EXT_swap_control)
		wglSwapInterval(1);

	if (GLExtensions::GLVersion >= GLExtensions::GL_4_3) {
		glEnable(GL_DEBUG_OUTPUT);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}

	return true;
#endif

	return false;
}

bool Win32Application::InitializeDirect3D9()
{
#ifdef DIRECT3D9
	if (NULL == (d3dinterface = Direct3DCreate9(D3D_SDK_VERSION)))
		return false;

	D3DPRESENT_PARAMETERS d3dpp;

	d3dpp.BackBufferFormat				= D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount				= 1;
	d3dpp.BackBufferHeight				= clientheight;
	d3dpp.BackBufferWidth				= clientwidth;
	d3dpp.AutoDepthStencilFormat		= D3DFMT_D24S8;
	d3dpp.hDeviceWindow					= hwnd;
	d3dpp.Windowed						= true;
	d3dpp.EnableAutoDepthStencil		= true;
	d3dpp.PresentationInterval			= D3DPRESENT_INTERVAL_ONE;
	d3dpp.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	d3dpp.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	d3dpp.MultiSampleType				= D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality			= 0;
	d3dpp.Flags							= 0;

	if (FAILED(d3dinterface->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)))
	{
		MessageBox(NULL, "Could not create Direct3D device", "Fatal error", MB_OK);
		return false;
	}

	return true;
#endif

	return false;
}

bool Win32Application::InitializeDirect3D10()
{
#ifdef DIRECT3D10
	DXGI_SWAP_CHAIN_DESC	swapchaindesc;
	DXGI_MODE_DESC			displaymode;
	D3D10_VIEWPORT			viewport;
	UINT					flags = 0;

	memset(&swapchaindesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));

	displaymode.Format					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	displaymode.Width					= clientwidth;
	displaymode.Height					= clientheight;
	displaymode.RefreshRate.Numerator	= 60;
	displaymode.RefreshRate.Denominator	= 1;

	swapchaindesc.BufferDesc			= displaymode;
	swapchaindesc.BufferDesc.Width		= clientwidth;
	swapchaindesc.BufferDesc.Height		= clientheight;
	swapchaindesc.BufferCount			= 1;
	swapchaindesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.OutputWindow			= hwnd;
	swapchaindesc.SampleDesc.Count		= 1;
	swapchaindesc.SampleDesc.Quality	= 0;
	swapchaindesc.Windowed				= true;

#if (_MSC_VER > 1600) && defined(_DEBUG)
	flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags,
		D3D10_SDK_VERSION, &swapchaindesc, &swapchain, &device)))
	{
		MessageBox(NULL, "Could not create Direct3D device", "Fatal error", MB_OK);
		return false;
	}

	viewport.Width		= clientwidth;
	viewport.Height		= clientheight;
	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;

	device->RSSetViewports(1, &viewport);
	return true;
#endif

	return false;
}

bool Win32Application::InitializeDirect3D11(bool srgb)
{
#ifdef DIRECT3D11
	DXGI_SWAP_CHAIN_DESC	swapchaindesc;
	DXGI_MODE_DESC			displaymode;
	D3D11_VIEWPORT			viewport;
	UINT					flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D3D_FEATURE_LEVEL		featurelevel = D3D_FEATURE_LEVEL_10_0;
	HRESULT					hr = S_OK;

	memset(&swapchaindesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));

	displaymode.Format					= (srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
	displaymode.Width					= clientwidth;
	displaymode.Height					= clientheight;
	displaymode.RefreshRate.Numerator	= 60;
	displaymode.RefreshRate.Denominator	= 1;

	swapchaindesc.BufferDesc			= displaymode;
	
	swapchaindesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.OutputWindow			= hwnd;
	swapchaindesc.SampleDesc.Count		= 1;
	swapchaindesc.SampleDesc.Quality	= 0;
	swapchaindesc.Windowed				= TRUE;

#ifdef DIRECT2D
	swapchaindesc.BufferCount			= 1;
	swapchaindesc.SwapEffect			= DXGI_SWAP_EFFECT_DISCARD;
#else
	swapchaindesc.BufferCount			= 2;
	swapchaindesc.SwapEffect			= DXGI_SWAP_EFFECT_SEQUENTIAL;
#endif

#if (_MSC_VER > 1600) && defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featurelevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	hr = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, featurelevels, ARRAY_SIZE(featurelevels),
		D3D11_SDK_VERSION, &swapchaindesc, &swapchain, &device, &featurelevel, &context);

	viewport.Width		= (FLOAT)clientwidth;
	viewport.Height		= (FLOAT)clientheight;
	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;

	if (context)
		context->RSSetViewports(1, &viewport);

	return SUCCEEDED(hr);
#endif

	return false;
}

bool Win32Application::InitializeDirect2D()
{
#ifdef DIRECT2D
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dfactory))) {
		MessageBox(NULL, "Could not create Direct2D factory", "Fatal error", MB_OK);
		return false;
	}

	if (FAILED(d2dfactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(clientwidth, clientheight)), &d2drendertarget))) {
		MessageBox(NULL, "Could not create Direct2D render target", "Fatal error", MB_OK);
		return false;
	}

	return true;
#endif

	return false;
}

bool Win32Application::InitializeVulkan()
{
#ifdef VULKAN
	const char* instancelayers[] = {
		"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_KHRONOS_validation",
//		"VK_LAYER_RENDERDOC_Capture"
	};

	const char* instanceextensions[] = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils"
		//"VK_EXT_debug_report"	// TODO: not part of standard
	};

	const char* deviceextensions[] = {
		"VK_KHR_swapchain"
	};

	VkApplicationInfo		app_info			= {};
	VkInstanceCreateInfo	inst_info			= {};
	VkLayerProperties*		layerprops			= nullptr;
	VkResult				res;

	uint32_t				numlayers			= 0;
	uint32_t				numsupportedlayers	= 0;
	uint32_t				numtestlayers		= ARRAY_SIZE(instancelayers);
	uint32_t				apiversion			= 0;

	PFN_vkEnumerateInstanceVersion test_EnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");

	if (test_EnumerateInstanceVersion == nullptr) {
		printf("Could not find Vulkan 1.1 compatible ICD\n");
		return false;
	}

	vkEnumerateInstanceVersion(&apiversion);

	if (VK_VERSION_MAJOR(apiversion) == 1 && VK_VERSION_MINOR(apiversion) < 1) {
		printf("Could not find Vulkan 1.1 compatible ICD\n");
		return false;
	}

	// search for layers
	vkEnumerateInstanceLayerProperties(&numlayers, 0);
	layerprops = new VkLayerProperties[numlayers];

	vkEnumerateInstanceLayerProperties(&numlayers, layerprops);

	for (uint32_t i = 0; i < numtestlayers;) {
		uint32_t found = UINT32_MAX;

		for (uint32_t j = 0; j < numlayers; ++j) {
			if (0 == strcmp(layerprops[j].layerName, instancelayers[i])) {
				found = j;
				break;
			}
		}

		if (found == UINT32_MAX) {
			// unsupported
			printf("Layer '%s' unsupported\n", instancelayers[i]);

			std::swap(instancelayers[i], instancelayers[numtestlayers - 1]);
			--numtestlayers;
		} else {
			++numsupportedlayers;
			++i;
		}
	}

	delete[] layerprops;

	// create instance
	app_info.sType						= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext						= NULL;
	app_info.pApplicationName			= "Asylum's shader sample";
	app_info.applicationVersion			= 1;
	app_info.pEngineName				= "Asylum's sample engine";
	app_info.engineVersion				= 1;
	app_info.apiVersion					= VK_API_VERSION_1_1;

	inst_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext						= NULL;
	inst_info.flags						= 0;
	inst_info.pApplicationInfo			= &app_info;
	inst_info.ppEnabledExtensionNames	= instanceextensions;
	inst_info.ppEnabledLayerNames		= instancelayers;

#ifdef ENABLE_VALIDATION
	inst_info.enabledExtensionCount		= ARRAY_SIZE(instanceextensions);
	inst_info.enabledLayerCount			= numsupportedlayers;
#else
	inst_info.enabledExtensionCount		= ARRAY_SIZE(instanceextensions) - 1;
	inst_info.enabledLayerCount			= 0;
#endif

	res = vkCreateInstance(&inst_info, NULL, &driverInfo.instance);

	V_RETURN(false, "Could not find Vulkan driver", res != VK_ERROR_INCOMPATIBLE_DRIVER);
	V_RETURN(false, "Some layers are not present, remove them from the code", res != VK_ERROR_LAYER_NOT_PRESENT);
	V_RETURN(false, "Some extensions are not present, remove them from the code", res != VK_ERROR_EXTENSION_NOT_PRESENT);
	V_RETURN(false, "Unknown error", res == VK_SUCCESS);

#ifdef ENABLE_VALIDATION
	driverInfo.vkCreateDebugUtilsMessengerEXT	= reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(driverInfo.instance, "vkCreateDebugUtilsMessengerEXT"));
	driverInfo.vkDestroyDebugUtilsMessengerEXT	= reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(driverInfo.instance, "vkDestroyDebugUtilsMessengerEXT"));

	VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo;

	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	callbackCreateInfo.pNext = NULL;
	callbackCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	//	| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	callbackCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	callbackCreateInfo.pfnUserCallback = &DebugUtilsReportCallback;
	callbackCreateInfo.flags = 0;
	callbackCreateInfo.pUserData = nullptr;

	driverInfo.vkCreateDebugUtilsMessengerEXT(driverInfo.instance, &callbackCreateInfo, nullptr, &driverInfo.messenger);

	/*
	driverInfo.vkCreateDebugReportCallbackEXT	= reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(driverInfo.instance, "vkCreateDebugReportCallbackEXT"));
	driverInfo.vkDebugReportMessageEXT			= reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(driverInfo.instance, "vkDebugReportMessageEXT"));
	driverInfo.vkDestroyDebugReportCallbackEXT	= reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(driverInfo.instance, "vkDestroyDebugReportCallbackEXT"));

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;

	callbackCreateInfo.sType		= VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfo.pNext		= nullptr;
	callbackCreateInfo.flags		= VK_DEBUG_REPORT_ERROR_BIT_EXT|VK_DEBUG_REPORT_WARNING_BIT_EXT|VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback	= &DebugReportCallback;
	callbackCreateInfo.pUserData	= nullptr;

	res = driverInfo.vkCreateDebugReportCallbackEXT(driverInfo.instance, &callbackCreateInfo, nullptr, &driverInfo.callback);
	VK_ASSERT(res == VK_SUCCESS);
	*/
#endif

	// enumerate devices
	res = vkEnumeratePhysicalDevices(driverInfo.instance, &driverInfo.numDevices, 0);
	VK_ASSERT(driverInfo.numDevices > 0);

	driverInfo.devices = new VkPhysicalDevice[driverInfo.numDevices];
	res = vkEnumeratePhysicalDevices(driverInfo.instance, &driverInfo.numDevices, driverInfo.devices);

	VkPhysicalDeviceProperties deviceprops;
	uint32_t discretegpu = UINT_MAX;
	uint32_t integratedgpu = UINT_MAX;
	uint32_t selecteddevice = UINT_MAX;
	
	for (uint32_t i = 0; i < driverInfo.numDevices; ++i) {
		vkGetPhysicalDeviceProperties(driverInfo.devices[i], &deviceprops);

		if (deviceprops.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			discretegpu = i;
		} else if (deviceprops.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			integratedgpu = i;
		}
	}

	selecteddevice = ((discretegpu == UINT_MAX) ? integratedgpu : discretegpu);
	V_RETURN(false, "Could not find discrete or integrated GPU", selecteddevice != UINT_MAX);

	driverInfo.selectedDevice = driverInfo.devices[selecteddevice];

	vkGetPhysicalDeviceQueueFamilyProperties(driverInfo.selectedDevice, &driverInfo.numQueues, 0);
	VK_ASSERT(driverInfo.numQueues > 0);

	driverInfo.queueProps = new VkQueueFamilyProperties[driverInfo.numQueues];
	vkGetPhysicalDeviceQueueFamilyProperties(driverInfo.selectedDevice, &driverInfo.numQueues, driverInfo.queueProps);

	// query standard features
	vkGetPhysicalDeviceProperties(driverInfo.selectedDevice, &driverInfo.deviceProps);
	vkGetPhysicalDeviceFeatures(driverInfo.selectedDevice, &driverInfo.deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(driverInfo.selectedDevice, &driverInfo.memoryProps);

	// query additional features
	driverInfo.deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	driverInfo.deviceFeatures2.pNext = NULL;

	vkGetPhysicalDeviceFeatures2(driverInfo.selectedDevice, &driverInfo.deviceFeatures2);

	// check version
	apiversion = driverInfo.deviceProps.apiVersion;

	if (VK_VERSION_MAJOR(apiversion) == 1 && VK_VERSION_MINOR(apiversion) < 1) {
		printf("Vulkan 1.1 not supported on the selected device\n");
		return false;
	}

	VkPhysicalDeviceProperties2 props2;

	driverInfo.subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
	driverInfo.subgroupProps.pNext = NULL;

	props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props2.pNext = &driverInfo.subgroupProps;

	vkGetPhysicalDeviceProperties2(driverInfo.selectedDevice, &props2);

	// create surface
	VkWin32SurfaceCreateInfoKHR createinfo = {};

	createinfo.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createinfo.pNext		= NULL;
	createinfo.hinstance	= hinst;
	createinfo.hwnd			= hwnd;

	driverInfo.numPresentModes = 0;

	res = vkCreateWin32SurfaceKHR(driverInfo.instance, &createinfo, NULL, &driverInfo.surface);
	VK_ASSERT(res == VK_SUCCESS);

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(driverInfo.selectedDevice, driverInfo.surface, &driverInfo.surfaceCaps);
	VK_ASSERT(res == VK_SUCCESS);

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(driverInfo.selectedDevice, driverInfo.surface, &driverInfo.numPresentModes, NULL);
	VK_ASSERT(driverInfo.numPresentModes > 0);

	driverInfo.presentModes = new VkPresentModeKHR[driverInfo.numPresentModes];

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(driverInfo.selectedDevice, driverInfo.surface, &driverInfo.numPresentModes, driverInfo.presentModes);
	VK_ASSERT(res == VK_SUCCESS);

	// check for a presentation queue
	VkBool32* presentok = new VkBool32[driverInfo.numQueues];

	for (uint32_t i = 0; i < driverInfo.numQueues; ++i) {
		vkGetPhysicalDeviceSurfaceSupportKHR(driverInfo.selectedDevice, i, driverInfo.surface, &presentok[i]);
	}

	driverInfo.graphicsQueueID = UINT32_MAX;
	driverInfo.computeQueueID = UINT32_MAX;

	for (uint32_t i = 0; i < driverInfo.numQueues; ++i) {
		if ((driverInfo.queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentok[i] == VK_TRUE)
			driverInfo.graphicsQueueID = i;

		if (driverInfo.queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			driverInfo.computeQueueID = i;
	}
	
	V_RETURN(false, "Adapter does not have a queue that supports graphics and presenting", driverInfo.graphicsQueueID != UINT32_MAX);

	delete[] presentok;

	// query formats
	uint32_t numformats;

	vkGetPhysicalDeviceSurfaceFormatsKHR(driverInfo.selectedDevice, driverInfo.surface, &numformats, 0);
	VK_ASSERT(numformats > 0);

	VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[numformats];

	res = vkGetPhysicalDeviceSurfaceFormatsKHR(driverInfo.selectedDevice, driverInfo.surface, &numformats, formats);
	VK_ASSERT(res == VK_SUCCESS);

	driverInfo.format = VK_FORMAT_UNDEFINED;

	for (uint32_t i = 0; i < numformats; ++i) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
			driverInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			break;
		}
	}

	if (driverInfo.format == VK_FORMAT_UNDEFINED)
		driverInfo.format = ((formats[0].format == VK_FORMAT_UNDEFINED) ? VK_FORMAT_B8G8R8A8_UNORM : formats[0].format);

	delete[] formats;

	// create device
	VkDeviceQueueCreateInfo	queueinfo		= {};
	VkDeviceCreateInfo		deviceinfo		= {};
	float					priorities[1]	= { 0.0f };

	queueinfo.sType						= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueinfo.pNext						= NULL;
	queueinfo.queueCount				= 1;
	queueinfo.pQueuePriorities			= priorities;
	queueinfo.queueFamilyIndex			= driverInfo.graphicsQueueID;

	deviceinfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceinfo.pNext					= &driverInfo.deviceFeatures2;
	deviceinfo.queueCreateInfoCount		= 1;
	deviceinfo.pQueueCreateInfos		= &queueinfo;
	deviceinfo.enabledExtensionCount	= ARRAY_SIZE(deviceextensions);
	deviceinfo.ppEnabledLayerNames		= instancelayers;
	deviceinfo.ppEnabledExtensionNames	= deviceextensions;
	deviceinfo.pEnabledFeatures			= NULL; //&driverInfo.deviceFeatures;

#ifdef ENABLE_VALIDATION
	deviceinfo.enabledLayerCount		= numsupportedlayers;
#else
	deviceinfo.enabledLayerCount		= 0;
#endif

	res = vkCreateDevice(driverInfo.selectedDevice, &deviceinfo, NULL, &driverInfo.device);
	VK_ASSERT(res == VK_SUCCESS);

	vkGetDeviceQueue(driverInfo.device, driverInfo.graphicsQueueID, 0, &driverInfo.graphicsQueue);
	VK_ASSERT(driverInfo.graphicsQueue != VK_NULL_HANDLE);
	
	// create command pool
	VkCommandPoolCreateInfo poolinfo = {};

	poolinfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolinfo.pNext				= NULL;
	poolinfo.queueFamilyIndex	= driverInfo.graphicsQueueID;
	poolinfo.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	res = vkCreateCommandPool(driverInfo.device, &poolinfo, NULL, &driverInfo.commandPool);
	VK_ASSERT(res == VK_SUCCESS);

	// create swapchain
	VkSwapchainCreateInfoKHR swapchaininfo = {};

	swapchaininfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	for (uint32_t i = 0; i < driverInfo.numPresentModes; ++i) {
		if (driverInfo.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			swapchaininfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		if (driverInfo.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			// mailbox is best
			swapchaininfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}

	swapchaininfo.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchaininfo.pNext					= NULL;
	swapchaininfo.surface				= driverInfo.surface;
	swapchaininfo.minImageCount			= 2;
	swapchaininfo.imageFormat			= driverInfo.format;
	swapchaininfo.imageExtent.width		= clientwidth;	// driverInfo.surfaceCaps.currentExtent.width
	swapchaininfo.imageExtent.height	= clientheight;	// driverInfo.surfaceCaps.currentExtent.height
	swapchaininfo.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchaininfo.compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchaininfo.imageArrayLayers		= 1;
	swapchaininfo.oldSwapchain			= VK_NULL_HANDLE;
	swapchaininfo.clipped				= true;
	swapchaininfo.imageColorSpace		= VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchaininfo.imageUsage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	swapchaininfo.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
	swapchaininfo.queueFamilyIndexCount	= 0;
	swapchaininfo.pQueueFamilyIndices	= NULL;

	res = vkCreateSwapchainKHR(driverInfo.device, &swapchaininfo, NULL, &driverInfo.swapchain);
	VK_ASSERT(res == VK_SUCCESS);

	res = vkGetSwapchainImagesKHR(driverInfo.device, driverInfo.swapchain, &driverInfo.numSwapChainImages, NULL);
	VK_ASSERT(res == VK_SUCCESS);

	driverInfo.swapchainImages = new VkImage[driverInfo.numSwapChainImages];
	driverInfo.swapchainImageViews = new VkImageView[driverInfo.numSwapChainImages];

	res = vkGetSwapchainImagesKHR(driverInfo.device, driverInfo.swapchain, &driverInfo.numSwapChainImages, driverInfo.swapchainImages);
	VK_ASSERT(res == VK_SUCCESS);

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		VkImageViewCreateInfo color_image_view = {};

		color_image_view.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext								= NULL;
		color_image_view.format								= driverInfo.format;
		color_image_view.components.r						= VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g						= VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b						= VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a						= VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel		= 0;
		color_image_view.subresourceRange.levelCount		= 1;
		color_image_view.subresourceRange.baseArrayLayer	= 0;
		color_image_view.subresourceRange.layerCount		= 1;
		color_image_view.viewType							= VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.flags								= 0;
		color_image_view.image								= driverInfo.swapchainImages[i];

		res = vkCreateImageView(driverInfo.device, &color_image_view, NULL, &driverInfo.swapchainImageViews[i]);
		VK_ASSERT(res == VK_SUCCESS);
	}

	presenter = new VulkanPresentationEngine(clientwidth, clientheight);
	glslang::InitializeProcess();

	return true;
#endif

	return false;
}

bool Win32Application::InitializeDriverInterface(GraphicsAPI api)
{
	switch (api) {
	case GraphicsAPIOpenGL:
		return InitializeOpenGL();

	case GraphicsAPIOpenGLDeprecated:
		return InitializeOpenGL(false);

	case GraphicsAPIDirect3D9:
		return InitializeDirect3D9();

	case GraphicsAPIDirect3D10:
		return InitializeDirect3D10();

	case GraphicsAPIDirect3D11:
#ifdef DIRECT2D
		return InitializeDirect3D11(false);
#else
		return InitializeDirect3D11(true);
#endif

	case GraphicsAPIDirect2D:
		return InitializeDirect2D();

	case GraphicsAPIVulkan:
		return InitializeVulkan();

	default:
		break;
	}

	return false;
}

void Win32Application::DestroyDriverInterface()
{
#ifdef OPENGL
	if (hglrc) {
		wglMakeCurrent(hdc, NULL);
		wglDeleteContext(hglrc);
	}

	if (hdc != nullptr)
		ReleaseDC(hwnd, hdc);

	hdc = nullptr;
	hglrc = nullptr;
#endif

#ifdef DIRECT3D11
	if (context) {
		context->ClearState();
		context->Release();
	}

	if (swapchain)
		swapchain->Release();

	if (device) {
		ULONG rc = device->Release();

		if (rc > 0)
			MessageBox(NULL, "You forgot to release something", "Direct3D leak", MB_OK);
	}

#	ifdef _DEBUG
	HMODULE debuglayer = GetModuleHandleW (L"Dxgidebug.dll");
	IDXGIDebug* dxgidebug = nullptr;

	typedef HRESULT (WINAPI *DXGIGETDEBUGINTERFACEPROC)(REFIID riid, void **ppDebug);
	
	if (debuglayer != NULL) {
		DXGIGETDEBUGINTERFACEPROC getdebuginterface = NULL;

		getdebuginterface = (DXGIGETDEBUGINTERFACEPROC)GetProcAddress (debuglayer, "DXGIGetDebugInterface");

		if (getdebuginterface != NULL && SUCCEEDED(getdebuginterface (IID_IDXGIDebug, (void**)&dxgidebug))) {
			dxgidebug->ReportLiveObjects (DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
			dxgidebug->Release ();
		}
	}
#	endif
#elif defined(DIRECT3D10)
	if (device)
		device->ClearState();

	if (swapchain)
		swapchain->Release();

	if (device) {
		ULONG rc = device->Release();

		if (rc > 0)
			MessageBox(NULL, "You forgot to release something", "Direct3D leak", MB_OK);
	}
#elif defined(DIRECT3D9)
	if (device) {
		ULONG rc = device->Release();

		if (rc > 0)
			MessageBox(NULL, "You forgot to release something", "Direct3D leak", MB_OK);
	}

	if (d3dinterface)
		d3dinterface->Release();
#endif

#ifdef DIRECT2D
	if (d2drendertarget)
		d2drendertarget->Release();

	if (d2dfactory) {
		ULONG rc = d2dfactory->Release();

		if (rc > 0)
			MessageBox(NULL, "You forgot to release something", "Direct2D leak", MB_OK);
	}
#endif

#ifdef VULKAN
	glslang::FinalizeProcess();

	delete presenter;

	VulkanContentRegistry::Release();
	VulkanMemorySubAllocator::Release();

	for (uint32_t i = 0; i < driverInfo.numSwapChainImages; ++i) {
		vkDestroyImageView(driverInfo.device, driverInfo.swapchainImageViews[i], 0);
	}

	delete[] driverInfo.swapchainImageViews;
	delete[] driverInfo.swapchainImages;
	delete[] driverInfo.presentModes;
	delete[] driverInfo.queueProps;
	delete[] driverInfo.devices;

	vkDestroySwapchainKHR(driverInfo.device, driverInfo.swapchain, 0);
	vkDestroyCommandPool(driverInfo.device, driverInfo.commandPool, 0);
	vkDestroyDevice(driverInfo.device, 0);
	vkDestroySurfaceKHR(driverInfo.instance, driverInfo.surface, 0);

#	ifdef ENABLE_VALIDATION
	driverInfo.vkDestroyDebugUtilsMessengerEXT(driverInfo.instance, driverInfo.messenger, 0);
	//driverInfo.vkDestroyDebugReportCallbackEXT(driverInfo.instance, driverInfo.callback, 0);
#	endif

	vkDestroyInstance(driverInfo.instance, NULL);
#endif
}

bool Win32Application::Present()
{
#ifdef OPENGL
	if (hdc)
		SwapBuffers(hdc);
#endif

#ifdef DIRECT3D9
	if (device)
		device->Present(NULL, NULL, NULL, NULL);
#endif

#if defined(DIRECT3D10) || defined(DIRECT3D11)
	if (swapchain)
		swapchain->Present(1, 0);
#endif

	return false;
}

void Win32Application::Run()
{
	if (InitSceneCallback != nullptr) {
		if (!InitSceneCallback())
			return;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	LARGE_INTEGER	qwTicksPerSec = { 0, 0 };
	LARGE_INTEGER	qwTime;
	LONGLONG		tickspersec;
	double			last, current;
	double			delta, accum = 0;
	MSG				msg;

	ZeroMemory(&msg, sizeof(msg));

	QueryPerformanceFrequency(&qwTicksPerSec);
	tickspersec = qwTicksPerSec.QuadPart;

	QueryPerformanceCounter(&qwTime);
	last = (qwTime.QuadPart % tickspersec) / (double)tickspersec;

	while (msg.message != WM_QUIT) {
		QueryPerformanceCounter(&qwTime);
		current = (qwTime.QuadPart % tickspersec) / (double)tickspersec;

		if (current < last)
			delta = ((1.0 + current) - last);
		else
			delta = (current - last);

		last = current;
		accum += delta;

		while (accum > 0.0333) {
			accum -= 0.0333;

			while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if (msg.message == WM_QUIT)
					break;
			}

			if (msg.message == WM_QUIT)
				break;

			if (UpdateCallback != nullptr)
				UpdateCallback(0.0333f);
		}

		if (msg.message != WM_QUIT && RenderCallback != nullptr)
			RenderCallback((float)accum / 0.0333f, (float)delta);
	}

	if (UninitSceneCallback != nullptr)
		UninitSceneCallback();

	DestroyDriverInterface();
}

void Win32Application::SetTitle(const char* title)
{
	if (hwnd)
		SetWindowText(hwnd, title);
}

void* Win32Application::GetHandle() const
{
	return hwnd;
}

void* Win32Application::GetDriverInterface() const
{
#ifdef DIRECT3D9
	return d3dinterface;
#endif

#ifdef DIRECT2D
	return d2dfactory;
#endif

#ifdef VULKAN
	return driverInfo.instance;
#endif

	return nullptr;
}

void* Win32Application::GetLogicalDevice() const
{
#if defined(DIRECT3D9) || defined(DIRECT3D10) || defined(DIRECT3D11)
	return device;
#endif

#ifdef VULKAN
	return driverInfo.device;
#endif

	return nullptr;
}

void* Win32Application::GetSwapChain() const
{
#if defined(DIRECT3D10) || defined(DIRECT3D11)
	return swapchain;
#endif

#ifdef DIRECT2D
	return d2drendertarget;
#endif

#ifdef VULKAN
	return presenter;
#endif

	return nullptr;
}

void* Win32Application::GetDeviceContext() const
{
#ifdef DIRECT3D11
	return context;
#endif

	return nullptr;
}
