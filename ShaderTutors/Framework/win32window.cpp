
#include <dwmapi.h>
#include <iostream>

#include "win32window.h"
#include "drawingitem.h"
#include "renderingcore.h"

Win32Window::WindowMap	Win32Window::handles;
WNDCLASSEX				Win32Window::wc;
std::mutex				Win32Window::handlesguard;

LRESULT WINAPI Win32Window::WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	Win32Window* window = nullptr;

	handlesguard.lock();
	{
		auto it = Win32Window::handles.find(hWnd);

		if (it != Win32Window::handles.end())
			window = it->second;
	}
	handlesguard.unlock();

	if (window == nullptr)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);

		if (window->CloseCallback != nullptr)
			window->CloseCallback(window);

		window->DeleteUniverse();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYUP:
		if (window->KeyUpCallback != nullptr)
			window->KeyUpCallback(window, (KeyCode)wParam);

		break;

	case WM_MOUSEMOVE: {
		short x = (short)(lParam & 0xffff);
		short y = (short)((lParam >> 16) & 0xffff);

		window->inputstate.dX = x - window->inputstate.X;
		window->inputstate.dY = y - window->inputstate.Y;

		window->inputstate.X = x;
		window->inputstate.Y = y;

		if (window->MouseMoveCallback != nullptr)
			window->MouseMoveCallback(window, window->inputstate.X, window->inputstate.Y, window->inputstate.dX, window->inputstate.dY);

		} break;

	case WM_LBUTTONDOWN:
		window->inputstate.Button |= MouseButtonLeft;

		if (window->MouseDownCallback)
			window->MouseDownCallback(window, MouseButtonLeft, window->inputstate.X, window->inputstate.Y);

		break;

	case WM_RBUTTONDOWN:
		window->inputstate.Button |= MouseButtonRight;

		if (window->MouseDownCallback)
			window->MouseDownCallback(window, MouseButtonRight, window->inputstate.X, window->inputstate.Y);

		break;

	case WM_LBUTTONUP:
		if (window->MouseUpCallback && (window->inputstate.Button & MouseButtonLeft))
			window->MouseUpCallback(window, MouseButtonLeft, window->inputstate.X, window->inputstate.Y);

		window->inputstate.Button &= (~MouseButtonLeft);
		break;

	case WM_RBUTTONUP:
		if (window->MouseUpCallback && (window->inputstate.Button & MouseButtonRight))
			window->MouseUpCallback(window, MouseButtonRight, window->inputstate.X, window->inputstate.Y);

		window->inputstate.Button &= (~MouseButtonRight);
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Win32Window::Win32Window(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	if (handles.size() == 0) {
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.style			= CS_OWNDC;
		wc.lpfnWndProc		= (WNDPROC)WndProc;
		wc.cbClsExtra		= 0L;
		wc.cbWndExtra		= 0L;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(0, IDC_ARROW);
		wc.hbrBackground	= NULL;
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= "AsylumSampleClass";
		wc.hIconSm			= NULL;

		RegisterClassEx(&wc);
	}

	RECT realrect;
	RECT rect = { (LONG)x, (LONG)y, (LONG)width, (LONG)height };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_SYSMENU|WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX;

	rect.left	= (LONG)x;
	rect.top	= (LONG)y;
	rect.right	= (LONG)(x + width);
	rect.bottom	= (LONG)(y + height);

	hwnd = CreateWindowA(
		"AsylumSampleClass", "Win32Window", style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);

	DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &realrect, sizeof(RECT));

	rect.left	= rect.left + (rect.left - realrect.left);
	rect.right	= rect.right - (realrect.right - rect.right);
	rect.top	= rect.top + (rect.top - realrect.top);
	rect.bottom	= rect.bottom - (realrect.bottom - rect.bottom);

	MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

	hdc			= NULL;
	universeID	= -1;
	drawingitem	= nullptr;

	if (hwnd) {
		hdc = GetDC(hwnd);
		universeID = GetRenderingCore()->CreateUniverse(hdc);

		if (universeID != -1) {
			drawingitem = new DrawingItem(
				universeID,
				(uint32_t)(rect.right - rect.left),
				(uint32_t)(rect.bottom - rect.top));
		}
	}

	std::lock_guard<std::mutex> lock(handlesguard);
	handles.insert(WindowMap::value_type(hwnd, this));
}

Win32Window::~Win32Window()
{
	std::lock_guard<std::mutex> lock(handlesguard);

	ReleaseDC(hwnd, hdc);
	handles.erase(hwnd);

	hwnd = NULL;
	hdc = NULL;

	if (handles.size() == 0)
		UnregisterClass("AsylumSampleClass", wc.hInstance);
}

void Win32Window::DeleteUniverse()
{
	if (drawingitem != nullptr)
		delete drawingitem;

	if (universeID != -1) {
		GetRenderingCore()->DeleteUniverse(universeID);
		universeID = -1;

		DestroyWindow(hwnd);
	}
}

void Win32Window::SetTitle(const char* title)
{
	if (hwnd)
		::SetWindowTextA(hwnd, title);
}

void Win32Window::SetFocus()
{
	if (hwnd)
		::SetFocus(hwnd);
}

void Win32Window::Close()
{
	if (hwnd)
		SendMessageA(hwnd, WM_CLOSE, 0, 0);
}

void Win32Window::MessageHook()
{
	LARGE_INTEGER	qwTicksPerSec = { 0, 0 };
	LARGE_INTEGER	qwTime;
	LONGLONG		tickspersec;
	double			last, current;
	double			delta, accum = 0;
	MSG				msg;
	bool			render = true;

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	if (CreateCallback != nullptr)
		CreateCallback(this);

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

				if (msg.message == WM_CLOSE)
					render = false;

				if (msg.message == WM_QUIT)
					break;
			}

			if (msg.message == WM_QUIT)
				break;

			if (render && UpdateCallback != nullptr)
				UpdateCallback(0.0333f);
		}

		if (render && msg.message != WM_QUIT) {
			if (RenderCallback != nullptr && universeID != -1)
				RenderCallback(this, (float)accum / 0.0333f, (float)delta);
		}
	}
}
