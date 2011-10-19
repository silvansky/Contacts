include(../../config.inc)

macx: {
  DEFINES += USE_FILE32API
}

TARGET         = minizip
TEMPLATE       = lib
CONFIG        -= qt
CONFIG        += staticlib warn_off
INCLUDEPATH   += ../..
DESTDIR        = ../../libs
include(minizip.pri)
