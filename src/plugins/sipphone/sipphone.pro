# works only on win and mac
win32-msvc2008|macx: {
	include(sipphone.pri)
	include(../plugins.inc)

	QT    += multimedia
	contains(DEFINES, USE_PHONON){
	  QT  += phonon dbus
	}

	INCLUDEPATH += ../../thirdparty/siplibraries/SipLib/inc
	INCLUDEPATH += ../../thirdparty/siplibraries/SPEEX/include
	INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/Inc/iLBC
	INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/inc
	INCLUDEPATH += ../../thirdparty/siplibraries/VoIPVideoLib/inc

} else: {
	include(../nobuild.inc)
}

win32-msvc2008: {
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/baseclasses/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/ffmpeg/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/sdllib/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/SipLib/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPMediaLib/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/VoIPVideoLib/lib

	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/pjsiplast/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/x264/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/directxsdk/lib

	LIBS  += -lWs2_32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -lwinmm
	LIBS  += -lIphlpapi -lIphlpapi -ldsound -ldxguid -lnetapi32 -lmswsock -luser32 -lgdi32 -ladvapi32

	CONFIG(debug, debug|release) {
	  LIBS  += -llibpjproject-i386-Win32-vc8-Debug
	} else {
	  LIBS  += -llibpjproject-i386-Win32-vc8-Release
	}
	INCLUDEPATH += ../../thirdparty/siplibraries/baseclasses/include
	INCLUDEPATH += ../../thirdparty/siplibraries/ffmpeg/include
	INCLUDEPATH += ../../thirdparty/siplibraries/sdllib/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjlib/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjlib-util/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjmedia/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjnath/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/pjsip/include
	INCLUDEPATH += ../../thirdparty/siplibraries/pjsip/x264/include
}
macx: {
	QMAKE_LFLAGS    += -framework Carbon -framework Cocoa -framework VideoDecodeAcceleration -framework QTKit -framework AVFoundation -framework CoreVideo -framework CoreMedia
	LIBS += -L/usr/local/lib
	INCLUDEPATH += /usr/local/include
	INCLUDEPATH += /usr/local/include/SDL
	# common
	LIBS += -lm -lpthread -lcrypto
	# openssl
	LIBS += -lssl
	# zlib
	LIBS += -lzlib
	# SDL
	LIBS += -lSDL
	# av
	LIBS += -lavcodec -lavdevice -lavfilter -lavformat -lavutil
	# swscale
	LIBS += -lswscale -lswresample
	# speex
	LIBS += -lspeex-i386-apple-darwin11.3.0
	# resample
	LIBS += -lresample-i386-apple-darwin11.3.0
	# iLBC
	LIBS += -lilbc
	# GSM
	LIBS += -lgsm
	# bz2
	LIBS += -lbz2
	# x264
	LIBS += -lx264
	# D-Bus
	LIBS += -ldbus-1
	# portaudio
	LIBS += -lportaudio-i386-apple-darwin11.3.0
	# pjsip
	LIBS += -lpj-i386-apple-darwin11.3.0
	LIBS += -lpjlib-util-i386-apple-darwin11.3.0
	LIBS += -lpjmedia-audiodev-i386-apple-darwin11.3.0
	LIBS += -lpjmedia-codec-i386-apple-darwin11.3.0
	LIBS += -lpjmedia-i386-apple-darwin11.3.0
	LIBS += -lpjmedia-videodev-i386-apple-darwin11.3.0
	LIBS += -lpjnath-i386-apple-darwin11.3.0
	LIBS += -lpjsip-i386-apple-darwin11.3.0
	LIBS += -lpjsip-simple-i386-apple-darwin11.3.0
	LIBS += -lpjsip-ua-i386-apple-darwin11.3.0
	LIBS += -lpjsua-i386-apple-darwin11.3.0
}
