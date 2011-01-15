
LIBS  += -L../../thirdparty/sipexternal/SipLib/lib
LIBS  += -L../../thirdparty/sipexternal/VoIPMediaLib/lib
LIBS  += -L../../thirdparty/sipexternal/VoIPVideoLib/lib
LIBS  += -lWs2_32 -lVoIPVideoLib -lVoIPMedia -lSipProtocol
INCLUDEPATH += ../../thirdparty/sipexternal/SipLib/inc ../../thirdparty/sipexternal/VoIPMediaLib/inc ../../thirdparty/sipexternal/VoIPVideoLib/inc
INCLUDEPATH += ../../thirdparty/sipexternal/SPEEX/include
INCLUDEPATH += ../../thirdparty/sipexternal/VoIPMediaLib/Inc/iLBC



QT    += multimedia
TARGET = sipphone 
include(sipphone.pri) 
include(../plugins.inc) 
