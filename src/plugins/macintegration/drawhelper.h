#ifndef DRAWHELPER_H
#define DRAWHELPER_H

#import <Cocoa/Cocoa.h>

// forward declaration of private class
@class NSThemeFrame;

// see http://zathras.de/programming/cocoa/UKCustomWindowFrame.zip/UKCustomWindowFrame/PrivateHeaders/NSThemeFrame.h
// for reversed private header

@interface DrawHelper : NSObject
{
}

// dummy declarations of some undocumented functions of NSThemeFrame and its superclasses
- (float)roundedCornerRadius;
- (NSWindow*)window;
- (id)_displayName;
- (NSRect)bounds;
- (NSString *)title;
- (NSSize)sizeOfTitlebarButtons;
- (NSPoint)_closeButtonOrigin;
- (NSPoint)_zoomButtonOrigin;
- (NSPoint)_collapseButtonOrigin;
- (void)_setTextShadow:(BOOL)on;

// methods to exchange with NSThemeFrame class
- (void)drawRect:(NSRect)rect;
- (void)_drawTitleStringIn:(NSRect)rect withColor:(NSColor *)color;
- (NSRect)_titlebarTitleRect;

// renamed original methods of NSThemeFrame class
- (void)drawRectOriginal:(NSRect)rect;
- (void)_drawTitleStringOriginalIn: (NSRect) rect withColor: (NSColor *) color;
- (NSRect)_titlebarTitleRectOriginal;

// methods to add to NSThemeFrame class
-(NSAttributedString*)attributedTitle;

@end

void setFrameColor(NSColor *color);
void setTitleColor(NSColor *color);

#endif // DRAWHELPER_H
