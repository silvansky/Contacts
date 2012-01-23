#ifndef DRAWHELPER_H
#define DRAWHELPER_H

#import <Cocoa/Cocoa.h>

@interface DrawHelper : NSObject
{
}

- (float)roundedCornerRadius;
- (void)drawRectOriginal:(NSRect)rect;
- (void)_drawTitleStringOriginalIn: (NSRect) rect withColor: (NSColor *) color;
- (NSWindow*)window;
- (id)_displayName;
- (NSRect)bounds;
- (NSString *)title;

- (void)drawRect:(NSRect)rect;
- (void) _drawTitleStringIn: (NSRect) rect withColor: (NSColor *) color;

@end

void setFrameColor(NSColor *color);
void setTitleColor(NSColor *color);

#endif // DRAWHELPER_H
