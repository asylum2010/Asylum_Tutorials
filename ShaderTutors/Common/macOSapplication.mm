
#import <Cocoa/Cocoa.h>

#if defined(METAL)
#	import "MetalViewController.h"
#elif defined(OPENGL)
#	import "GLViewController.h"
#endif

#import "macOSapplication.h"

macOSApplication::macOSApplication(uint32_t width, uint32_t height)
{
	// NOTE: in pixels
	clientwidth = width;
	clientheight = height;
}

macOSApplication::~macOSApplication()
{
}

bool macOSApplication::InitializeDriverInterface(GraphicsAPI api)
{
#if defined(METAL)
	metalDevice = MTLCreateSystemDefaultDevice();
	
	if (!metalDevice) {
		NSLog(@"Metal is not supported on this device");
		return false;
	}
#endif
	
	return true;
}

bool macOSApplication::Present()
{
	return false;
}

void macOSApplication::Run()
{
	const char* argv[] = { "AsylumSample" };
	
	@autoreleasepool {
		NSApplicationMain(1, argv);
	}
}

void macOSApplication::SetTitle(const char* title)
{
	windowtitle = [NSString stringWithUTF8String:title];
}

uint8_t macOSApplication::GetMouseButtonState() const
{
#if defined(METAL)
	return [(MetalView*)metalView getMouseButtonState];
#elif defined(OPENGL)
	return [(GLView*)metalView getMouseButtonState];
#endif
}

void* macOSApplication::GetLogicalDevice() const
{
#if defined(METAL)
	return (void*)CFBridgingRetain(metalDevice);
#else
	return nullptr;
#endif
}

void* macOSApplication::GetSwapChain() const
{
	return (void*)CFBridgingRetain(metalView);
}

