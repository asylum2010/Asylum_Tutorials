
#ifndef _MACOSAPPLICATION_H_
#define _MACOSAPPLICATION_H_

#import <MetalKit/MetalKit.h>
#import "application.h"

class macOSApplication : public Application
{
private:
	NSString* windowtitle;
	uint32_t clientwidth;
	uint32_t clientheight;
	
public:
	MTKView* metalView;
	id<MTLDevice> metalDevice;
	
	macOSApplication(uint32_t width, uint32_t height);
	~macOSApplication();
	
	bool InitializeDriverInterface(GraphicsAPI api) override;
	bool Present() override;
	
	void Run() override;
	void SetTitle(const char* title) override;
	
	uint8_t GetMouseButtonState() const override;
	
	uint32_t GetClientWidth() const override		{ return clientwidth; }
	uint32_t GetClientHeight() const override		{ return clientheight; }
	
	void* GetDriverInterface() const override		{ return nullptr; }
	void* GetLogicalDevice() const override			{ return (void*)CFBridgingRetain(metalDevice); }
	void* GetSwapChain() const override				{ return (void*)CFBridgingRetain(metalView); }
	
	//specific
	NSString* GetTitle() const						{ return windowtitle; }
};

#endif
