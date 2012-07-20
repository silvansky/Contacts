TARGET = notifications 
contains(DEFINES, USE_PHONON) {
  QT  += phonon
}
include(notifications.pri)
include(../plugins.inc)
