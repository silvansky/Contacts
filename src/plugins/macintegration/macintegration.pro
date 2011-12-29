!macx: {
	error(This plugin can only be built on Mac OS X!)
}

TARGET = macintegration
include(macintegration.pri)
include(../plugins.inc)

# frameworks
# note: put Growl.framework & Sparkle.framework to /Library/Frameworks/

LIBS += -framework Cocoa -framework Growl -framework Sparkle

INCLUDEPATH += /Library/Frameworks/Growl.framework/Headers \
			   /Library/Frameworks/Sparkle.framework/Headers

QT += webkit

