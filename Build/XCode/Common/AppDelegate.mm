
#import "AppDelegate.h"
#import "macOSapplication.h"

extern Application* app;

@implementation WindowController

- (void)windowDidLoad {
	NSWindow* window = self.window;
	NSRect rect = window.contentView.frame;
	
	rect.size.width = app->GetClientWidth();
	rect.size.height = app->GetClientHeight();
	
	rect = [[NSScreen mainScreen] convertRectFromBacking:rect];
	
	NSRect framerect = [window frameRectForContentRect:rect];
	NSRect desktoprect = [NSScreen mainScreen].frame;
	
	framerect.origin.x = desktoprect.origin.x + (desktoprect.size.width - framerect.size.width) * 0.5f;
	framerect.origin.y = desktoprect.origin.y + (desktoprect.size.height - framerect.size.height) * 0.5f;

	[window setFrame:framerect display:YES animate:YES];
	[window setTitle:((macOSApplication*)app)->GetTitle()];
}

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
	return YES;
}

@end
