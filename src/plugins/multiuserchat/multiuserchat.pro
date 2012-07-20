TARGET = multiuserchat
include(multiuserchat.pri)
include(../plugins.inc)
!CONFIG(debug, debug|release) {
  INSTALLS =
}
