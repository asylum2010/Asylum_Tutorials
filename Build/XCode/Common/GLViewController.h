
#import <Cocoa/Cocoa.h>
#import <cstdint>

@interface GLView : NSOpenGLView

- (uint8_t)getMouseButtonState;
- (void)DEBUG_RecreateContext;

@end

@interface GLViewController : NSViewController

@end
