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

