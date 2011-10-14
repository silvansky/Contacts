FORMS = notifywidget.ui \
	  notifykindswidget.ui

HEADERS = notifications.h \
	  notifywidget.h \
	  notifytextbrowser.h \
	  notifykindswidget.h \
	  notifykindswidgets.h

SOURCES = notifications.cpp \
	  notifywidget.cpp \
	  notifytextbrowser.cpp \
	  notifykindswidget.cpp \
	  notifykindswidgets.cpp

macx: {
	HEADERS += growlpreferences.h
	SOURCES += growlpreferences.cpp
	FORMS += growlpreferences.ui
}
