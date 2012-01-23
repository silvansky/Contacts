#include "drawhelper.h"

static NSColor * gFrameColor = nil;
static NSColor * gTitleColor = nil;

@implementation DrawHelper

#define CLIP_METHOD2

- (void)drawRect:(NSRect)rect
{
	// Call original drawing method
	[self drawRectOriginal:rect];
	[self _setTextShadow:NO];

	NSRect titleRect;

	// Build clipping path
#ifndef CLIP_METHOD2
	NSRect windowRect = [[self window] frame];
	windowRect.origin = NSMakePoint(0, 0);

	float cornerRadius = [self roundedCornerRadius];
	[[NSBezierPath bezierPathWithRoundedRect:windowRect xRadius:cornerRadius yRadius:cornerRadius] addClip];
	[[NSBezierPath bezierPathWithRect:rect] addClip];
	titleRect = NSMakeRect(0, 0, windowRect.size.width, windowRect.size.height);
#else
	NSRect brect = [self bounds];

	float radius = [self roundedCornerRadius];
	NSBezierPath *path = [[NSBezierPath alloc] init];
	NSPoint topMid = NSMakePoint(NSMidX(brect), NSMaxY(brect));
	NSPoint topLeft = NSMakePoint(NSMinX(brect), NSMaxY(brect));
	NSPoint topRight = NSMakePoint(NSMaxX(brect), NSMaxY(brect));
	NSPoint bottomRight = NSMakePoint(NSMaxX(brect), NSMinY(brect));

	[path moveToPoint: topMid];
	[path appendBezierPathWithArcFromPoint: topRight toPoint: bottomRight radius: radius];
	[path appendBezierPathWithArcFromPoint: bottomRight toPoint: brect.origin radius: radius];
	[path appendBezierPathWithArcFromPoint: brect.origin toPoint: topLeft radius: radius];
	[path appendBezierPathWithArcFromPoint: topLeft toPoint: topRight radius: radius];
	[path closePath];

	[path addClip];
	[path release];
	titleRect = NSMakeRect(0, 0, brect.size.width, brect.size.height);
#endif

	CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	CGContextSetBlendMode(context, kCGBlendModeMultiply);

	// background
	if (gFrameColor == nil)
	{
		NSLog(@"frame color is nil, setting default");
		gFrameColor = [[NSColor colorWithCalibratedRed: (65 / 255.0) green: (70 / 255.0) blue: (77 / 255.0) alpha: 1.0] retain];
	}

	[gFrameColor set];

#ifndef CLIP_METHOD2
	[[NSBezierPath bezierPathWithRect:rect] fill];
#else
	NSRect headerRect = NSIntersectionRect(NSMakeRect(brect.origin.x, brect.origin.y + brect.size.height - 22, brect.size.width, 22), rect);
	[[NSBezierPath bezierPathWithRect:headerRect] fill];
#endif

	CGContextSetBlendMode(context, kCGBlendModeCopy);
	// draw title text
	[self _drawTitleStringIn: titleRect withColor: nil];
}

- (void)_drawTitleStringIn: (NSRect) rect withColor: (NSColor *) color
{
	if (!gTitleColor)
		gTitleColor = [[NSColor colorWithCalibratedRed: .6 green: .6 blue: .6 alpha: 1.0] retain];
	//[self _drawTitleStringOriginalIn: rect withColor: gTitleColor];

	NSLog(@"title: %@", [self title]);

	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont fontWithName:@"SegoeUI" size:10], NSFontAttributeName, gTitleColor, NSForegroundColorAttributeName, nil];
	NSAttributedString * str = [[NSAttributedString alloc] initWithString:[self title] attributes:attributes];
	[str drawWithRect:rect options:NSStringDrawingTruncatesLastVisibleLine];
}

@end


void setFrameColor(NSColor *color)
{
	if (gFrameColor)
		[gFrameColor release];
	gFrameColor = [color copy];
}

void setTitleColor(NSColor *color)
{
	if (gTitleColor)
		[gTitleColor release];
	gTitleColor = [color copy];
}
