
#import "GLViewController.h"
#import "macOSapplication.h"
#import "glextensions.h"

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

@implementation GLView
{
	InputState _inputState;
	NSTrackingArea* _trackingArea;
	NSOpenGLPixelFormat* _format;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
	
	if (self) {
		NSOpenGLPixelFormatAttribute attributes[] = {
			NSOpenGLPFAColorSize, 24,
			//NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, 24,
			NSOpenGLPFAStencilSize, 8,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFANoRecovery,
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
			0
		};
		
		_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
		NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:_format shareContext:nil];
		
		if (context == nil) {
			NSLog(@"Fallback to compatibility profile!");
			
			// try comptibility profile
			attributes[11] = (NSOpenGLPixelFormatAttribute)0;
			
			_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
			context = [[NSOpenGLContext alloc] initWithFormat:_format shareContext:nil];
		}
		
		[self setOpenGLContext:context];
		[context makeCurrentContext];
		
		GLExtensions::QueryFeatures();
		
		[self DEBUG_RecreateContext];
		
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

- (void)DEBUG_RecreateContext
{
	@autoreleasepool {
		NSOpenGLContext* current = self.openGLContext;

		[NSOpenGLContext clearCurrentContext];
		[self setOpenGLContext:nil];
		
		NSLog(@"Recreating GL context...");
		
		NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:_format shareContext:current];
		
		if (context)
			NSLog(@"OK");
		else
			NSLog(@"FAIL");
		
		[self setOpenGLContext:context];
		
		[context update];
		[context makeCurrentContext];
	}
}

- (void)drawRect:(NSRect)dirtyRect
{
	[self.openGLContext flushBuffer];
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

@implementation GLViewController
{
	GLView* _view;
	NSTimer* _timer;
	macOSApplication* _app;
}

- (void)viewDidLoad {
	[super viewDidLoad];

	_app = (macOSApplication*)app;
	_timer = [NSTimer timerWithTimeInterval:(1.0f / 60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];
	
	[[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSEventTrackingRunLoopMode];
	
	_view = (GLView*)self.view;
	//[_view reshape];
	
	_app->metalView = _view;
	
	if (_app->InitSceneCallback != nullptr)
		_app->InitSceneCallback();
}

- (void)setRepresentedObject:(id)representedObject {
	[super setRepresentedObject:representedObject];

	// Update the view, if already loaded.
}

- (void)animationTimer:(NSTimer*)timer {
	if (_app->UpdateCallback != nullptr)
		_app->UpdateCallback(1.0f / 60.0f);
	
	if (_app->RenderCallback != nullptr)
		_app->RenderCallback(0, 1.0f / 60.0f);
	
	[self.view setNeedsDisplay:YES];
}

@end
