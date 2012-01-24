#ifndef DRAWHELPER_H
#define DRAWHELPER_H

#import <Cocoa/Cocoa.h>

@interface DrawHelper : NSObject
{
}

// dummy declarations of some undocumented functions of NSThemeFrame and its superclasses
- (float)roundedCornerRadius;
- (void)drawRectOriginal:(NSRect)rect;
- (void)_drawTitleStringOriginalIn: (NSRect) rect withColor: (NSColor *) color;
- (NSWindow*)window;
- (id)_displayName;
- (NSRect)bounds;
- (NSString *)title;
- (NSSize)sizeOfTitlebarButtons;
- (NSPoint)_closeButtonOrigin;
- (NSPoint)_zoomButtonOrigin;
- (NSPoint)_collapseButtonOrigin;
- (void)_setTextShadow:(BOOL)on;

- (void)drawRect:(NSRect)rect;
- (void) _drawTitleStringIn: (NSRect) rect withColor: (NSColor *) color;

@end

void setFrameColor(NSColor *color);
void setTitleColor(NSColor *color);

#endif // DRAWHELPER_H
