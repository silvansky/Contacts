win32: {
	TARGET = win7integration
	include(win7integration.pri)
	include(../plugins.inc)
} else {
	include(../nobuild.inc)
}
