SOURCES += stylesheeteditorplugin.cpp \
    stylesheeteditor.cpp \
    csshighlighter.cpp \
    qdesigner_utils.cpp \
    teststylesform.cpp \
    private/qcssparser.cpp \
    private/qcssscanner.cpp
HEADERS += stylesheeteditorplugin.h \
    stylesheeteditor.h \
    csshighlighter.h \
    qdesigner_utils.h \
    teststylesform.h \
    private/qcssparser_p.h
include(qtgradienteditor/qtgradienteditor.pri)

FORMS += \
    teststylesform.ui
