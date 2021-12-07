
#ifndef _MACOSAPPLICATION_H_
#define _MACOSAPPLICATION_H_

#if defined(METAL)
#	import <MetalKit/MetalKit.h>
#else
#	import <Cocoa/Cocoa.h>
#endif

#import "application.h"

class macOSApplication : public Application
{
private:
	NSString* windowtitle;
	uint32_t clientwidth;
	uint32_t clientheight;
	
public:
#if defined(METAL)
	MTKView* metalView;
	id<MTLDevice> metalDevice;
#elif defined(OPENGL)
	NSView* metalView;	// didn't plan to use GL again...
#endif
	
	macOSApplication(uint32_t width, uint32_t height);
	~macOSApplication();
	
	bool InitializeDriverInterface(GraphicsAPI api) override;
	bool Present() override;
	
	void Run() override;
	void SetTitle(const char* title) override;
	
	uint8_t GetMouseButtonState() const override;
	
	uint32_t GetClientWidth() const override		{ return clientwidth; }
	uint32_t GetClientHeight() const override		{ return clientheight; }
	
	void* GetHandle() const override				{ return nullptr; }
	void* GetDriverInterface() const override		{ return nullptr; }
	void* GetLogicalDevice() const override;
	void* GetSwapChain() const override;
	void* GetDeviceContext() const override;		{ return nullptr; }
	
	//specific
	NSString* GetTitle() const						{ return windowtitle; }
};

#endif
