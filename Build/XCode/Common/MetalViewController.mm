
#import "MetalViewController.h"
#import "macOSapplication.h"

extern Application* app;

struct InputState
{
	uint8_t Button;
	int16_t X, Y;
	int32_t dX, dY, dZ;
};

static NSPoint convertScreenToBase(NSWindow* window, NSPoint pt)
{
	NSRect rc = NSMakeRect(pt.x, pt.y, 1, 1);
	
	if (window)
		rc = [window convertRectFromScreen:rc];
	
	return rc.origin;
}

@implementation MetalView
{
	InputState _inputState;
	NSTrackingArea* _trackingArea;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
	
	if (self) {
		_trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
													 options:NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveInKeyWindow
													   owner:self
													userInfo:nil];
		
		[self addTrackingArea:_trackingArea];
		[self setTranslatesAutoresizingMaskIntoConstraints:YES];
		
		_inputState.Button = 0;
		_inputState.X = _inputState.Y = 0;
		_inputState.dX = _inputState.dY = 0;
	}
	
	return self;
}

- (void)updateTrackingAreas {
	[self removeTrackingArea:_trackingArea];
	
	_trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
												 options:NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveInKeyWindow
												   owner:self
												userInfo:nil];
	
	[self addTrackingArea:_trackingArea];
	[super updateTrackingAreas];
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (void)keyDown:(NSEvent *)event {
	if (app->KeyDownCallback)
		app->KeyDownCallback((KeyCode)[event keyCode]);
}

- (void)keyUp:(NSEvent *)event {
	if ([event keyCode] == KeyCodeEscape) {
		[[NSApp mainWindow] close];
		return;
	}
	
	if (app->KeyUpCallback)
		app->KeyUpCallback((KeyCode)[event keyCode]);
}

- (void)mouseMoved:(NSEvent*)theEvent {
	NSPoint pos = convertScreenToBase(self.window, [NSEvent mouseLocation]);
	
	_inputState.dX = (short)[theEvent deltaX];
	_inputState.dY = (short)[theEvent deltaY];
	
	_inputState.X = (short)pos.x;
	_inputState.Y = (short)(self.bounds.size.height - pos.y);
	
	if (app != nullptr) {
		if (app->MouseMoveCallback != nullptr)
			app->MouseMoveCallback(_inputState.X, _inputState.Y, _inputState.dX, _inputState.dY);
	}

	[super mouseMoved:theEvent];
}

- (void)mouseDragged:(NSEvent*)theEvent {
	NSPoint pos = convertScreenToBase(self.window, [NSEvent mouseLocation]);
	
	_inputState.Button |= MouseButtonLeft;
	
	_inputState.dX = (short)[theEvent deltaX];
	_inputState.dY = (short)[theEvent deltaY];
	
	_inputState.X = (short)pos.x;
	_inputState.Y = (short)(self.bounds.size.height - pos.y);
	
	if (app != nullptr) {
		if (app->MouseMoveCallback != nullptr)
			app->MouseMoveCallback(_inputState.X, _inputState.Y, _inputState.dX, _inputState.dY);
	}
	
	[super mouseDragged:theEvent];
}

- (void)rightMouseDragged:(NSEvent*)theEvent {
	NSPoint pos = convertScreenToBase(self.window, [NSEvent mouseLocation]);
	
	_inputState.Button |= MouseButtonRight;
	
	_inputState.dX = (short)[theEvent deltaX];
	_inputState.dY = (short)[theEvent deltaY];
	
	_inputState.X = (short)pos.x;
	_inputState.Y = (short)(self.bounds.size.height - pos.y);
	
	if (app != nullptr) {
		if (app->MouseMoveCallback != nullptr)
			app->MouseMoveCallback(_inputState.X, _inputState.Y, _inputState.dX, _inputState.dY);
	}
	
	[super rightMouseDragged:theEvent];
}

- (void)mouseDown:(NSEvent*)theEvent {
	_inputState.Button |= MouseButtonLeft;
	
	if (app != nullptr) {
		if (app->MouseDownCallback != nullptr)
			app->MouseDownCallback(MouseButtonLeft, _inputState.X, _inputState.Y);
	}
	
	[super mouseDown:theEvent];
}

- (void)rightMouseDown:(NSEvent*)theEvent {
	_inputState.Button |= MouseButtonRight;
	
	if (app != nullptr) {
		if (app->MouseDownCallback != nullptr)
			app->MouseDownCallback(MouseButtonRight, _inputState.X, _inputState.Y);
	}
	
	[super rightMouseDown:theEvent];
}

- (void)mouseUp:(NSEvent*)theEvent {
	_inputState.Button &= (~MouseButtonLeft);
	
	if (app != nullptr) {
		if (app->MouseUpCallback != nullptr)
			app->MouseUpCallback(MouseButtonLeft, _inputState.X, _inputState.Y);
	}
	
	[super mouseUp:theEvent];
}

- (void)rightMouseUp:(NSEvent*)theEvent {
	_inputState.Button &= (~MouseButtonRight);
	
	if (app != nullptr) {
		if (app->MouseUpCallback != nullptr)
			app->MouseUpCallback(MouseButtonRight, _inputState.X, _inputState.Y);
	}
	
	[super rightMouseUp:theEvent];
}

- (uint8_t)getMouseButtonState {
	return _inputState.Button;
}

@end

@implementation MetalViewController
{
	MTKView* _view;
	macOSApplication* _app;
}

- (void)viewDidLoad {
	[super viewDidLoad];

	_app = (macOSApplication*)app;
	_view = (MTKView*)self.view;
	
	_view.device = _app->metalDevice;
	_app->metalView = _view;
	
	if (!_view.device) {
		self.view = [[NSView alloc] initWithFrame:self.view.frame];
		return;
	}
	
	_view.delegate = self;

	_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
	_view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
	_view.sampleCount = 1;
	
	_view.clearColor = MTLClearColorMake(0.0f, 0.0103f, 0.0707f, 1.0f);
	_view.clearDepth = 1;
	_view.clearStencil = 0;
	
	[self mtkView:_view drawableSizeWillChange:_view.bounds.size];
	
	if (_app->InitSceneCallback != nullptr)
		_app->InitSceneCallback();
}

- (void)setRepresentedObject:(id)representedObject {
	[super setRepresentedObject:representedObject];

	// Update the view, if already loaded.
}

- (void)drawInMTKView:(nonnull MTKView *)view {
	if (_app->UpdateCallback != nullptr)
		_app->UpdateCallback(1.0f / view.preferredFramesPerSecond);
	
	if (_app->RenderCallback != nullptr)
		_app->RenderCallback(0, 1.0f / view.preferredFramesPerSecond);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
	// TODO:
}

@end
