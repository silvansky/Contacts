#import <Cocoa/Cocoa.h>

#define COCOA_CLASSES_DEFINED
#import "macwidgets.h"

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

bool isWindowGrowButtonEnabled(QWidget * window)
{
    if (window)
        return [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] isEnabled] == YES;
    else
        return false;
}

void setWindowGrowButtonEnabled(QWidget * window, bool enabled)
{
    if (window)
        [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] setEnabled: (enabled ? YES : NO)];
}
