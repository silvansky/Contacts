# works only on win and mac
win32-msvc2008|macx: {
	TARGET = sipphone
	include(sipphone.pri)
	include(../plugins.inc)

	QT    += multimedia
	contains(DEFINES, USE_PHONON){
	  QT  += phonon
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
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/x264/lib
	LIBS  += -L$${_PRO_FILE_PWD_}/../../thirdparty/siplibraries/directxsdk/lib
	
	LIBS  += -L$${_PRO_FILE_PWD_}/../../../../pjsip_mod/lib

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
	INCLUDEPATH += ../../thirdparty/siplibraries/x264/include
	
	INCLUDEPATH += ../../../../pjsip_mod/pjlib/include
	INCLUDEPATH += ../../../../pjsip_mod/pjlib-util/include
	INCLUDEPATH += ../../../../pjsip_mod/pjmedia/include
	INCLUDEPATH += ../../../../pjsip_mod/pjnath/include
	INCLUDEPATH += ../../../../pjsip_mod/pjsip/include
}

macx: {
	contains(DEFINES, USE_PHONON) {
	  QT  += dbus
	}

	CONFIG += thread debug
	QMAKE_CXXFLAGS += -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -DPJMEDIA_VIDEO_DEV_HAS_CALLBACK=1 -I/usr/local/include/SDL -D_THREAD_SAFE -DPJMEDIA_HAS_LIBAVDEVICE=1 -DPJMEDIA_HAS_LIBAVFORMAT=1 -DPJMEDIA_HAS_LIBAVCODEC=1 -DPJMEDIA_HAS_LIBSWSCALE=1 -DPJMEDIA_HAS_LIBAVUTIL=1 -DPJMEDIA_VIDEO_DEV_HAS_QT=1

	DARWIN_VER = $$system("uname -r")
	PLATFORM = $$system("uname -p")
	LIB_SUFIX = $${PLATFORM}-apple-darwin$${DARWIN_VER}
	DARWIN10 = $$system("[ `uname -r` \\< \"11.0.0\" ] && echo true || echo false")

	QMAKE_LFLAGS    += -framework Carbon -framework Cocoa
	QMAKE_LFLAGS    += -framework ForceFeedback -framework IOKit
	QMAKE_LFLAGS    += -framework VideoDecodeAcceleration -framework QTKit
	QMAKE_LFLAGS    += -framework CoreVideo -framework OpenGL
	contains(DARWIN10, "true") {
		# Mac OS X 10.6+ frameworks
	} else {
		# Mac OS X 10.7+ frameworks
		QMAKE_LFLAGS    += -framework AVFoundation -framework CoreMedia
	}
	LIBS += -L/usr/local/lib
	INCLUDEPATH += /usr/local/include
	INCLUDEPATH += /usr/local/include/SDL
	# common
	LIBS += -lm -lpthread -lcrypto
	# openssl
	LIBS += -lssl
	# zlib
	LIBS += -lzlib
	# iconv
	LIBS += -liconv
	# av
	LIBS += -lavcodec -lavdevice -lavfilter -lavformat -lavutil
	# swscale
	LIBS += -lswscale -lswresample
	# speex
	LIBS += -lspeex-$${LIB_SUFIX}
	# resample
	LIBS += -lresample-$${LIB_SUFIX}
	# iLBC
	#LIBS += -lilbc
	# GSM
	#LIBS += -lgsm
	# bz2
	LIBS += -lbz2
	# x264
	LIBS += -lx264
	# D-Bus
	LIBS += -ldbus-1
	# portaudio
	LIBS += -lportaudio-$${LIB_SUFIX}
	# pjsip
	LIBS += -lpj-$${LIB_SUFIX}
	LIBS += -lpjlib-util-$${LIB_SUFIX}
	LIBS += -lpjmedia-audiodev-$${LIB_SUFIX}
	LIBS += -lpjmedia-codec-$${LIB_SUFIX}
	LIBS += -lpjmedia-$${LIB_SUFIX}
	LIBS += -lpjmedia-videodev-$${LIB_SUFIX}
	LIBS += -lpjnath-$${LIB_SUFIX}
	LIBS += -lpjsip-$${LIB_SUFIX}
	LIBS += -lpjsip-simple-$${LIB_SUFIX}
	LIBS += -lpjsip-ua-$${LIB_SUFIX}
	LIBS += -lpjsua-$${LIB_SUFIX}
}
