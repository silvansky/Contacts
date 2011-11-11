#import "macwidgets.h"

#import <Cocoa/Cocoa.h>

static NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

WindowRef windowRefFromWidget(QWidget * w)
{
	return (WindowRef)[nsWindowFromWidget(w) windowRef];
}

NSWindow * nsWindowFromWidget(QWidget * w)
{
	return [nsViewFromWidget(w) window];
}

NSView * nsViewFromWidget(QWidget * w)
{
	return (NSView *)w->winId();
}

void setWindowShadowEnabled(QWidget * window, bool enabled)
{
	[[nsViewFromWidget(window) window] setHasShadow: (enabled ? YES : NO)];
}

void setWindowGrowButtonEnabled(QWidget * window, bool enabled)
{
	[[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] setEnabled: (enabled ? YES : NO)];
}
