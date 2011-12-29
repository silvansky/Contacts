#import "dockoverlayhelper.h"

@implementation DockOverlayHelper

@synthesize overlayImage = img, alignment = align;

- (void)drawRect:(NSRect) rect
{
	Q_UNUSED(rect)
	NSRect boundary = [self bounds];
	// draw app icon
	[[NSApp applicationIconImage] drawInRect:boundary fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
	if (self.overlayImage)
	{
		NSRect imgRect = NSMakeRect(0, 0, [self.overlayImage size].width, [self.overlayImage size].height);
		[self.overlayImage drawInRect:imgRect fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
	}
}

@end
