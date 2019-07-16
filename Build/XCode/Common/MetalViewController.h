
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <cstdint>

@interface MetalView : MTKView

- (uint8_t)getMouseButtonState;

@end

@interface MetalViewController : NSViewController <MTKViewDelegate>

@end
