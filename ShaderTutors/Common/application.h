
#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <functional>

#ifdef _WIN32
#	define PLATFORM_KEYCODE(win, mac)	win
#elif defined (__APPLE__)
#	define PLATFORM_KEYCODE(win, mac)	mac
#else
#endif

enum GraphicsAPI
{
	GraphicsAPIOpenGL = 0,			// core profile
	GraphicsAPIOpenGLDeprecated,	// compatibility profile
	GraphicsAPIDirect3D9,
	GraphicsAPIDirect3D10,
	GraphicsAPIDirect3D11,
	GraphicsAPIDirect2D,
	GraphicsAPIVulkan,
	GraphicsAPIMetal
};

enum MouseButton
{
	MouseButtonLeft = 1,
	MouseButtonRight = 2,
	MouseButtonMiddle = 4
};

enum KeyCode
{
	// https://snipplr.com/view/42797
	
	KeyCodeBackspace = 0x08,
	KeyCodeTab = PLATFORM_KEYCODE(0x09, 0x30),
	KeyCodeEnter = PLATFORM_KEYCODE(0x0d, 0x24),
	KeyCodeEscape = PLATFORM_KEYCODE(0x1b, 0x35),
	KeyCodeSpace = PLATFORM_KEYCODE(0x20, 0x31),
	KeyCode0 = PLATFORM_KEYCODE(0x30, 0x1D),
	KeyCode1 = PLATFORM_KEYCODE(0x31, 0x12),
	KeyCode2 = PLATFORM_KEYCODE(0x32, 0x13),
	KeyCode3 = PLATFORM_KEYCODE(0x33, 0x14),
	KeyCode4 = PLATFORM_KEYCODE(0x34, 0x15),
	KeyCode5 = PLATFORM_KEYCODE(0x35, 0x17),
	KeyCode6 = PLATFORM_KEYCODE(0x36, 0x16),
	KeyCode7 = PLATFORM_KEYCODE(0x37, 0x1A),
	KeyCode8 = PLATFORM_KEYCODE(0x38, 0x1C),
	KeyCode9 = PLATFORM_KEYCODE(0x39, 0x19),
	KeyCodeA = PLATFORM_KEYCODE(0x41, 0x00),
	KeyCodeB = PLATFORM_KEYCODE(0x42, 0x0B),
	KeyCodeC = PLATFORM_KEYCODE(0x43, 0x08),
	KeyCodeD = PLATFORM_KEYCODE(0x44, 0x02),
	KeyCodeE = PLATFORM_KEYCODE(0x45, 0x0E),
	KeyCodeF = PLATFORM_KEYCODE(0x46, 0x03),
	KeyCodeG = PLATFORM_KEYCODE(0x47, 0x05),
	KeyCodeH = PLATFORM_KEYCODE(0x48, 0x04),
	KeyCodeI = PLATFORM_KEYCODE(0x49, 0x22),
	KeyCodeJ = PLATFORM_KEYCODE(0x4a, 0x26),
	KeyCodeK = PLATFORM_KEYCODE(0x4b, 0x28),
	KeyCodeL = PLATFORM_KEYCODE(0x4c, 0x25),
	KeyCodeM = PLATFORM_KEYCODE(0x4d, 0x2E),
	KeyCodeN = PLATFORM_KEYCODE(0x4e, 0x2D),
	KeyCodeO = PLATFORM_KEYCODE(0x4f, 0x1F),
	KeyCodeP = PLATFORM_KEYCODE(0x50, 0x23),
	KeyCodeQ = PLATFORM_KEYCODE(0x51, 0x0C),
	KeyCodeR = PLATFORM_KEYCODE(0x52, 0x0F),
	KeyCodeS = PLATFORM_KEYCODE(0x53, 0x01),
	KeyCodeT = PLATFORM_KEYCODE(0x54, 0x11),
	KeyCodeU = PLATFORM_KEYCODE(0x55, 0x20),
	KeyCodeV = PLATFORM_KEYCODE(0x56, 0x09),
	KeyCodeW = PLATFORM_KEYCODE(0x57, 0x0D),
	KeyCodeX = PLATFORM_KEYCODE(0x58, 0x07),
	KeyCodeY = PLATFORM_KEYCODE(0x59, 0x10),
	KeyCodeZ = PLATFORM_KEYCODE(0x5a, 0x06),
	KeyCodeMultiply = 0x6a,
	KeyCodeAdd = 0x6b,
	KeyCodeSubtract = 0x6d,
	KeyCodeDivide = 0x6f,
	KeyCodeLeftShift = 0xa0,
	KeyCodeRightShift = 0xa1,
	KeyCodeLeftControl = 0xa2,
	KeyCodeRightControl = 0xa3,
	KeyCodeLeftAlt = 0xa4,
	KeyCodeRightAlt = 0xa5
};

class Application
{
public:
	std::function<bool ()> InitSceneCallback;
	std::function<void ()> UninitSceneCallback;
	std::function<void (float)> UpdateCallback;
	std::function<void (float, float)> RenderCallback;
	std::function<void (KeyCode)> KeyDownCallback;
	std::function<void (KeyCode)> KeyUpCallback;
	std::function<void (int32_t, int32_t, int16_t, int16_t)> MouseMoveCallback;
	std::function<void (int32_t, int32_t, int16_t)> MouseScrollCallback;
	std::function<void (MouseButton, int32_t, int32_t)> MouseDownCallback;
	std::function<void (MouseButton, int32_t, int32_t)> MouseUpCallback;

	virtual ~Application();

	virtual bool InitializeDriverInterface(GraphicsAPI api) = 0;
	virtual bool Present() = 0;

	virtual void Run() = 0;
	virtual void SetTitle(const char* title) = 0;

	virtual uint8_t GetMouseButtonState() const = 0;
	virtual uint32_t GetClientWidth() const = 0;
	virtual uint32_t GetClientHeight() const = 0;
	
	virtual void* GetHandle() const = 0;
	virtual void* GetDriverInterface() const = 0;
	virtual void* GetLogicalDevice() const = 0;
	virtual void* GetSwapChain() const = 0;
	virtual void* GetDeviceContext() const = 0;

	static Application* Create(uint32_t width, uint32_t height);
};

#endif
