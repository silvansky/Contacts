macx: {
	TARGET = macintegration
	include(macintegration.pri)
	include(../plugins.inc)

	# frameworks
	# note: put Growl.framework & Sparkle.framework to /Library/Frameworks/
	# IMPORTANT! use Growl SDK 1.3.1 or later to prevent app crashes (even with Growl 1.2.x installed in your system)

	LIBS += -framework Cocoa -framework Growl -framework Sparkle

	INCLUDEPATH += /Library/Frameworks/Growl.framework/Headers \
				   /Library/Frameworks/Sparkle.framework/Headers

	QT += webkit
} else {
	include(../nobuild.inc)
}
