
LIBS  += -L../../thirdparty/siplibraries/SipLib/lib
LIBS  += -L../../thirdparty/siplibraries/VoIPMediaLib/lib
LIBS  += -L../../thirdparty/siplibraries/VoIPVideoLib/lib
LIBS  += -lWs2_32
LIBS += -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -lwinmm

CONFIG(debug, debug|release) {
    LIBS  += -lVoIPVideoLibD -lVoIPMediaD -lSipProtocolD
} else {
    LIBS  += -lVoIPVideoLib -lVoIPMedia -lSipProtocol
}


INCLUDEPATH += ../../thirdparty/siplibraries/SipLib/inc ../../thirdparty/siplibraries/VoIPMediaLib/inc ../../thirdparty/siplibraries/VoIPVideoLib/inc
INCLUDEPATH += ../../thirdparty/siplibraries/SPEEX/include
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/Inc/iLBC



QT    += multimedia
QT    += phonon
TARGET = sipphone
include(sipphone.pri)
include(../plugins.inc)
