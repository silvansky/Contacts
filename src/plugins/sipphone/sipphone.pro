TARGET = sipphone
include(sipphone.pri)
include(../plugins.inc)

QT    += multimedia
USE_PHONON {
  QT  += phonon
}

LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/ffmpeg/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/sdllib/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/SipLib/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPMediaLib/lib
LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPVideoLib/lib

LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/pjsip/lib

LIBS  += -lWs2_32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -lwinmm
LIBS  += -lIphlpapi.lib -lIphlpapi.lib -ldsound.lib -ldxguid.lib -lnetapi32.lib -lmswsock.lib -luser32.lib -lgdi32.lib -ladvapi32.lib

CONFIG(debug, debug|release) {
//  LIBS  += -lVoIPVideoLibD -lVoIPMediaD -lSipProtocolD -llibpjproject-i386-Win32-vc8-Debug.lib
  LIBS  += -llibpjproject-i386-Win32-vc8-Debug.lib
} else {
//  LIBS  += -lVoIPVideoLib -lVoIPMedia -lSipProtocol
  LIBS  += -llibpjproject-i386-Win32-vc8-Release.lib
}


INCLUDEPATH += ../../thirdparty/siplibraries/SipLib/inc 
INCLUDEPATH += ../../thirdparty/siplibraries/SPEEX/include
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/Inc/iLBC
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/inc 
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPVideoLib/inc



INCLUDEPATH += ../../thirdparty/siplibraries/ffmpeg/include
INCLUDEPATH += ../../thirdparty/siplibraries/sdllib/include
INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjlib/include
INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjlib-util/include
INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjmedia/include
INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjnath/include
INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjsip/include








