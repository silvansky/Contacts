TARGET = console
include(console.pri)
include(../plugins.inc)
# TODO: remove this for release
!macx {
  INSTALLS =
}
