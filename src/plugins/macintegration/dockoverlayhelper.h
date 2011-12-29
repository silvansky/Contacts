#ifndef DOCKOVERLAYHELPER_H
#define DOCKOVERLAYHELPER_H

#import <Cocoa/Cocoa.h>
#include <Qt>

@interface DockOverlayHelper : NSView
{
	NSImage * img;
	Qt::Alignment align;
}

@property (nonatomic, retain) NSImage * overlayImage;
@property (nonatomic, assign) Qt::Alignment alignment;

@end

#endif // DOCKOVERLAYHELPER_H
