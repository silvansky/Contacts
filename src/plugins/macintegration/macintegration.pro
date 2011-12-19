TARGET = macintegration
include(macintegration.pri)
include(../plugins.inc)
QMAKE_LFLAGS    += -framework Cocoa

# note: put Growl.framework & Sparkle.framework to /Library/Frameworks/

LIBS += -framework Growl -framework Sparkle

INCLUDEPATH += /Library/Frameworks/Growl.framework/Headers \
			   /Library/Frameworks/Sparkle.framework/Headers

QT += webkit
