
#import <Cocoa/Cocoa.h>

#import "MetalViewController.h"
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
	metalDevice = MTLCreateSystemDefaultDevice();
	
	if (!metalDevice) {
		NSLog(@"Metal is not supported on this device");
		return false;
	}
	
	return true;
}

bool macOSApplication::Present()
{
	return false;
}

void macOSApplication::Run()
{
	const char* argv[] = { "AsylumSample" };
	NSApplicationMain(1, argv);
}

void macOSApplication::SetTitle(const char* title)
{
	windowtitle = [NSString stringWithUTF8String:title];
}

uint8_t macOSApplication::GetMouseButtonState() const
{
	return [(MetalView*)metalView getMouseButtonState];
}
