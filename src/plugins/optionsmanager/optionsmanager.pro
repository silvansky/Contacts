TARGET = optionsmanager
LIBS         += -L../../libs
LIBS         += -lqtlockedfile
win32:LIBS   += -luser32
include(optionsmanager.pri)
include(../plugins.inc)
