#import "dockoverlayhelper.h"

@implementation DockOverlayHelper

@synthesize overlayImage = img, alignment = align, drawAppIcon;

- (void)drawRect:(NSRect) rect
{
	Q_UNUSED(rect)
	NSRect boundary = [self bounds];
	if (self.drawAppIcon)
	{
		// draw app icon
		[[NSApp applicationIconImage] drawInRect:boundary fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
	}
	if (self.overlayImage)
	{
		CGFloat x = 0, y = 0;
		NSRect imgRect = NSMakeRect(0, 0, [self.overlayImage size].width, [self.overlayImage size].height);
		if (self.alignment & Qt::AlignLeft) // left
			x = 0;
		else if (self.alignment & Qt::AlignRight) // right
			x = boundary.size.width - imgRect.size.width;
		else // center
			x = (boundary.size.width - imgRect.size.width) / 2.0;
		if (self.alignment & Qt::AlignBottom) // bottom
			y = 0;
		else if (self.alignment & Qt::AlignTop) // top
			y = boundary.size.height - imgRect.size.height;
		else
			y = (boundary.size.height - imgRect.size.height) / 2.0;
		imgRect.origin = NSMakePoint(x, y);
		[self.overlayImage drawInRect:imgRect fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
	}
}

- (void)set
{
	[[NSApp dockTile] setContentView: self];
	[self update];
}

- (void)unset
{
	[[NSApp dockTile] setContentView: nil];
	[self update];
}

- (void)update
{
	[[NSApp dockTile] display];
}

@end
