HEADERS += \
	macintegrationplugin.h \
	macintegration_p.h

SOURCES += \
	macintegrationplugin.cpp

OBJECTIVE_HEADERS += \
	growl/GrowlTicketController.h \
	growl/GrowlPreferencesController.h \
	growl/GrowlPathUtilities.h \
	growl/GrowlDefinesInternal.h \
	growl/GrowlDefines.h \
	growl/GrowlApplicationBridge.h \
	growl/CFURLAdditions.h \
	growl/CFMutableDictionaryAdditions.h \
	growl/CFGrowlDefines.h \
	growl/CFGrowlAdditions.h \
	growl/GrowlPathway.h


OBJECTIVE_SOURCES += \
	macintegration.mm \
	growl/CFURLAdditions.c \
	growl/CFMutableDictionaryAdditions.c \
	growl/CFGrowlAdditions.c \
	growl/GrowlPathUtilities.m \
	growl/GrowlApplicationBridge.m

QT += webkit
