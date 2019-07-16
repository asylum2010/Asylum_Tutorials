
#ifndef _WIN32WINDOW_H_
#define _WIN32WINDOW_H_

#include <Windows.h>
#include <map>
#include <mutex>

#include "../Common/application.h"

class DrawingItem;

class Win32Window
{
	struct InputState
	{
		uint8_t Button;
		int16_t X, Y;
		int32_t dX, dY, dZ;
	};

	typedef std::map<HWND, Win32Window*> WindowMap;

	static WNDCLASSEX	wc;
	static WindowMap	handles;
	static std::mutex	handlesguard;

	static LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

private:
	HWND			hwnd;
	HDC				hdc;
	DrawingItem*	drawingitem;

	InputState		inputstate;
	uint32_t		clientwidth, clientheight;
	int				universeID;

	void DeleteUniverse();

public:
	std::function<void (Win32Window*)> CreateCallback;
	std::function<void (Win32Window*)> CloseCallback;
	std::function<void (float)> UpdateCallback;
	std::function<void (Win32Window*, float, float)> RenderCallback;
	std::function<void (Win32Window*, KeyCode)> KeyUpCallback;
	std::function<void (Win32Window*, int32_t, int32_t, int16_t, int16_t)> MouseMoveCallback;
	std::function<void (Win32Window*, MouseButton, int32_t, int32_t)> MouseDownCallback;
	std::function<void (Win32Window*, MouseButton, int32_t, int32_t)> MouseUpCallback;

	Win32Window(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	~Win32Window();

	void Close();
	void MessageHook();
	void SetTitle(const char* title);
	void SetFocus();

	inline DrawingItem* GetDrawingItem()	{ return drawingitem; }
	inline uint32_t GetClientWidth() const	{ return clientwidth; }
	inline uint32_t GetClientHeight() const	{ return clientheight; }
};

#endif
