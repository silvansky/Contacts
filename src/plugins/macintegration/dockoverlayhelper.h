#ifndef DOCKOVERLAYHELPER_H
#define DOCKOVERLAYHELPER_H

#import <Cocoa/Cocoa.h>
#include <Qt>

@interface DockOverlayHelper : NSView
{
	NSImage * img;
	int /*Qt::Alignment*/ align;
	BOOL drawAppIcon;
}

@property (nonatomic, retain) NSImage * overlayImage;
@property (nonatomic, assign) int /*Qt::Alignment*/ alignment;
@property (nonatomic, assign) BOOL drawAppIcon;

- (void)set;
- (void)unset;
- (void)update;

@end

#endif // DOCKOVERLAYHELPER_H
