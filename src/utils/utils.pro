include(../config.inc)
include(../install.inc)

TARGET             = $$TARGET_UTILS
VERSION            = $$VERSION_UTILS

TEMPLATE           = lib
CONFIG            += dll
QT                += xml webkit network
DEFINES           += UTILS_DLL
LIBS              += -L../libs
LIBS              += -lidn -lminizip -lzlib -lidle
macx: {
  QMAKE_LFLAGS    += -framework Carbon -framework Cocoa
} else:unix {
  LIBS            += -lXss
  CONFIG          += x11
} else:win32 {
  LIBS            += -luser32 -lComdlg32
  DLLDESTDIR       = ..\\..
  QMAKE_DISTCLEAN += $${DLLDESTDIR}\\$${TARGET}.dll
}
DEPENDPATH        += ..
INCLUDEPATH       += .. ../thirdparty/zlib
DESTDIR            = ../libs
include(utils.pri)

#Windows resources
win32:RC_FILE      = utils.rc

#Install (for Mac OS X - in loader.pro)
!macx:{
  target.path      = $$INSTALL_LIBS
  INSTALLS        += target

  #Translation
  TRANS_SOURCE_ROOT = ..
  include(../translations.inc)
}
