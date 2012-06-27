TARGET = notifications 
include(notifications.pri)
include(../plugins.inc)

contains(DEFINES, USE_PHONON) {
  QT  += phonon
}
