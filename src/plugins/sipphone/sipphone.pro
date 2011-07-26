TARGET = sipphone
include(sipphone.pri)
include(../plugins.inc)

QT    += multimedia
USE_PHONON {
  QT  += phonon
}

LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/SipLib/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPMediaLib/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPVideoLib/lib
LIBS  += -lWs2_32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -lwinmm

CONFIG(debug, debug|release) {
  LIBS  += -lVoIPVideoLibD -lVoIPMediaD -lSipProtocolD
} else {
  LIBS  += -lVoIPVideoLib -lVoIPMedia -lSipProtocol
}

INCLUDEPATH += ../../thirdparty/siplibraries/SipLib/inc 
INCLUDEPATH += ../../thirdparty/siplibraries/SPEEX/include
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/Inc/iLBC
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/inc 
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPVideoLib/inc

