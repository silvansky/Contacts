TARGET = optionsmanager
CONFIG       += x11
LIBS         += -L../../libs
LIBS         += -lqtlockedfile
win32:LIBS   += -luser32
macx: LIBS   += -framework Carbon
include(optionsmanager.pri)
include(../plugins.inc)
