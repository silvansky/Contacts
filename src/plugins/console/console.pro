TARGET = console
include(console.pri)
include(../plugins.inc)
!contains(DEFINES, WITH_CONSOLE) {
    INSTALLS =
}
