
#ifndef _WIN32APPLICATION_H_
#define _WIN32APPLICATION_H_

#include <Windows.h>

#ifdef OPENGL
#	include <gl/GL.h>
#endif

#ifdef DIRECT3D9
#	include <d3d9.h>
#endif

#ifdef DIRECT3D10
#	include <d3d10.h>
#endif

#ifdef DIRECT3D11
#	include <dxgi1_2.h>
#	include <d3d11_1.h>

#	ifdef _DEBUG
#		include <dxgidebug.h>
#	endif
#endif

#ifdef DIRECT2D
#	include <d2d1.h>
#endif

#ifdef VULKAN
#	define VK_USE_PLATFORM_WIN32_KHR
#	include "vk1ext.h"
#endif

#include "application.h"

class Win32Application : public Application
{
	struct InputState
	{
		uint8_t Button;
		int16_t X, Y;
		int32_t dX, dY, dZ;
	};

	friend LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

private:
#ifdef OPENGL
	HGLRC						hglrc;
#endif

#ifdef DIRECT3D9
	LPDIRECT3D9					d3dinterface;
	LPDIRECT3DDEVICE9			device;
#endif

#ifdef DIRECT3D10
	ID3D10Device*				device;
	IDXGISwapChain*				swapchain;
#endif

#ifdef DIRECT3D11
	ID3D11Device*				device;
	ID3D11DeviceContext*		context;
	IDXGISwapChain*				swapchain;
#endif

#ifdef DIRECT2D
	ID2D1Factory*				d2dfactory;
	ID2D1HwndRenderTarget*		d2drendertarget;
#endif

#ifdef VULKAN
	VulkanPresentationEngine*	presenter;
#endif

	HINSTANCE					hinst;
	HWND						hwnd;
	HDC							hdc;
	ULONG_PTR					gdiplustoken;

	InputState					inputstate;
	uint32_t					clientwidth;
	uint32_t					clientheight;

	bool InitializeOpenGL(bool core = true);
	bool InitializeDirect3D9();
	bool InitializeDirect3D10();
	bool InitializeDirect3D11(bool srgb);
	bool InitializeDirect2D();
	bool InitializeVulkan();

	void InitWindow();
	void AdjustWindow(RECT& out, uint32_t& width, uint32_t& height, DWORD style, DWORD exstyle, bool menu = false);
	void DestroyDriverInterface();

public:
	Win32Application(uint32_t width, uint32_t height);
	~Win32Application();

	bool InitializeDriverInterface(GraphicsAPI api) override;
	bool Present() override;

	void Run() override;
	void SetTitle(const char* title) override;

	uint8_t GetMouseButtonState() const override	{ return inputstate.Button; }
	uint32_t GetClientWidth() const override		{ return clientwidth; }
	uint32_t GetClientHeight() const override		{ return clientheight; }

	void* GetHandle() const override;
	void* GetDriverInterface() const override;
	void* GetLogicalDevice() const override;
	void* GetSwapChain() const override;
	void* GetDeviceContext() const override;
};

#endif
