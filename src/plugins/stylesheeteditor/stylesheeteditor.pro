TARGET = stylesheeteditor
include(stylesheeteditor.pri)
include(../plugins.inc)
!contains(DEFINES, WITH_SSEDITOR) {
	INSTALLS =
}
