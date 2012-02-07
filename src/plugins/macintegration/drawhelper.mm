#include "drawhelper.h"

static NSColor * gFrameColor = nil;
static NSColor * gTitleColor = nil;

@implementation DrawHelper

#define CLIP_METHOD2

/* dummy functions implementation - to prevent compiler warnings */

- (float)roundedCornerRadius { return 0; }

- (NSWindow*)window { return nil; }

- (id)_displayName { return nil; }

- (NSRect)bounds { return NSRect(); }

- (NSString *)title { return nil; }

- (NSSize)sizeOfTitlebarButtons { return NSSize(); }

- (NSPoint)_closeButtonOrigin { return NSPoint(); }

- (NSPoint)_zoomButtonOrigin { return NSPoint(); }

- (NSPoint)_collapseButtonOrigin { return NSPoint(); }

- (void)_setTextShadow:(BOOL)on { (void)on; }


/* real functions */

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

	titleRect = NSMakeRect(brect.origin.x, brect.origin.y + brect.size.height - 22, brect.size.width, 22);
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
	(void)rect;
	if (!color)
	{
		if (!gTitleColor)
			gTitleColor = [[NSColor colorWithCalibratedRed: .6 green: .6 blue: .6 alpha: 1.0] retain];

		NSAttributedString * str = [self attributedTitle];

		NSRect textRect = [self _titlebarTitleRect];

		[str drawWithRect:textRect options: NSStringDrawingUsesLineFragmentOrigin];
		[str release];
	}
	else
	{
		//NSLog(@"system text color: %@", color);
	}
}

- (NSRect)_titlebarTitleRect
{
	NSAttributedString * str = [self attributedTitle];
	NSRect brect = [self bounds];
	NSRect titleRect = NSMakeRect(brect.origin.x, brect.origin.y + brect.size.height - 22, brect.size.width, 22);

	NSSize tbSz = [self sizeOfTitlebarButtons];
	NSPoint pt = [self _zoomButtonOrigin];
	NSSize strSize = [str size];
	[str release];

	CGFloat dx = pt.x + tbSz.width + 6;
	CGFloat dy = 0.0;

	CGFloat titleLeft, centeredLeft = (brect.size.width - strSize.width) / 2.0;

	if (centeredLeft > dx)
		titleLeft = titleRect.origin.x + centeredLeft;
	else
		titleLeft = titleRect.origin.x + dx;

	NSRect textRect = NSMakeRect(titleLeft, titleRect.origin.y + dy, titleRect.size.width - dx, 0.0 /*titleRect.size.height - dy*/);
	return textRect;
}

-(NSAttributedString*)attributedTitle
{
	NSFont * font = [NSFont fontWithName:@"SegoeUI" size:16.0];

	NSMutableParagraphStyle * paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
	[paragraphStyle setLineBreakMode: NSLineBreakByTruncatingTail];

	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
			font, NSFontAttributeName,
			paragraphStyle, NSParagraphStyleAttributeName,
			gTitleColor, NSForegroundColorAttributeName,
			nil];

	NSAttributedString * str = [[NSAttributedString alloc] initWithString:[self title] attributes:attributes];
	[paragraphStyle release];
	return str;
}

/* renamed function dummy implementations */

- (void)drawRectOriginal:(NSRect)rect { (void)rect; }
- (void)_drawTitleStringOriginalIn: (NSRect) rect withColor: (NSColor *) color { (void)rect; (void)color; }
- (NSRect)_titlebarTitleRectOriginal { return NSRect(); }

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
