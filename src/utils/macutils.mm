#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CFString.h>

#define COCOA_CLASSES_DEFINED
#import "macutils.h"
#include <QDebug>

//static NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

WindowRef windowRefFromWidget(QWidget *w)
{
	return (WindowRef)[nsWindowFromWidget(w) windowRef];
}

NSWindow * nsWindowFromWidget(QWidget *w)
{
	return [nsViewFromWidget(w) window];
}

NSView * nsViewFromWidget(QWidget *w)
{
	return (NSView *)w->winId();
}

// warning! NSImage isn't released!
NSImage * nsImageFromQImage(const QImage &image)
{
	if (!image.isNull())
	{
		CGImageRef ref = QPixmap::fromImage(image).toMacCGImageRef();
		NSImage * nsimg = [[NSImage alloc] initWithCGImage: ref size: NSZeroSize];
		CGImageRelease(ref);
		return nsimg;
	}
	else
		return nil;
}

QImage qImageFromNSImage(NSImage *image)
{
	if (image)
	{
		CGImageRef ref = [image CGImageForProposedRect:NULL context:nil hints:nil];
		QImage result = QPixmap::fromMacCGImageRef(ref).toImage();
		return result;
	}
	return QImage();
}

// warning! NSString isn't released!
NSString * nsStringFromQString(const QString &s)
{
	const char * utf8String = s.toUtf8().constData();
	return [[NSString alloc] initWithUTF8String: utf8String];
}

QString qStringFromNSString(NSString *s)
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	QString res = QString::fromUtf8([s UTF8String]);
	[pool release];
	return res;
}

void setWindowShadowEnabled(QWidget *window, bool enabled)
{
	[[nsViewFromWidget(window) window] setHasShadow: (enabled ? YES : NO)];
}

bool isWindowGrowButtonEnabled(QWidget *window)
{
    if (window)
        return [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] isEnabled] == YES;
    else
        return false;
}

void setWindowGrowButtonEnabled(QWidget *window, bool enabled)
{
    if (window)
        [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] setEnabled: (enabled ? YES : NO)];
}

void hideWindow(void */* (NSWindow*) */ window)
{
	NSWindow *nsWindow = (NSWindow*)window;
	[nsWindow orderOut:nil];
}

QString convertFromMacCyrillic(const char *str)
{
	NSString *srcString = [[NSString alloc] initWithCString:str encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingMacCyrillic)];
	QString resString = qStringFromNSString(srcString);
	[srcString release];
	return resString;
}
