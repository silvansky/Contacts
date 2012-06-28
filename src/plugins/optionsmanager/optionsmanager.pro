TARGET = optionsmanager
unix:!macx:{
 CONFIG      += x11
}
QT           += webkit
LIBS         += -L../../libs
LIBS         += -lqtlockedfile
win32:LIBS   += -luser32
macx: LIBS   += -framework Carbon
include(optionsmanager.pri)
include(../plugins.inc)
