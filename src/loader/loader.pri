FORMS   = setuppluginsdialog.ui \
	  aboutbox.ui \
	  commentdialog.ui

HEADERS = pluginmanager.h \
	  setuppluginsdialog.h \
	  aboutbox.h \
	  commentdialog.h \
	  proxystyle.h

SOURCES = main.cpp \
	  pluginmanager.cpp \
	  setuppluginsdialog.cpp \
	  aboutbox.cpp \
	  commentdialog.cpp \
	  proxystyle.cpp

win32: {
SOURCES += \
	HoldemUtils/RUserLight.cpp \
	HoldemUtils/RShutDownManager.cpp \
	HoldemUtils/RNamedObjectsHelper.cpp \
	HoldemUtils/RHoldemModule.cpp \
	HoldemUtils/RGlobalLock.cpp \
	HoldemUtils/objectshutdownmanager.cpp

HEADERS += \
	HoldemUtils/RUserLight.h \
	HoldemUtils/RShutDownManager.h \
	HoldemUtils/RNamedObjectsHelper.h \
	HoldemUtils/RHoldemModule.h \
	HoldemUtils/RGlobalLock.h \
	HoldemUtils/HoldemUtilsConfig.h \
	HoldemUtils/Common/smart_any_fwd.h \
	HoldemUtils/Common/scoped_ptr.h \
	HoldemUtils/Common/scoped_any.h \
	HoldemUtils/Common/rdebug.h \
	HoldemUtils/Common/rbase.h \
	HoldemUtils/stdafx.h \
	HoldemUtils/objectshutdownmanager.h

LIBS += -lAdvapi32 -lUser32 -lOle32

}
