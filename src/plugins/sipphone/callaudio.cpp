#include <stdio.h>
#include <sys/types.h>
//#include <unistd.h>
#include <signal.h>
#include <qsettings.h>
#include <qmessagebox.h>

#include <QProcess>

//#include "config.h"

// Протокол SIP
#include "sipprotocol.h"
#include "sipcall.h"

// Библиотека Media
#include "sdp.h"
#include "dspaudioout.h"
#include "dspaudioin.h"
#include "dspoutrtp.h"
//#include "dspoutoss.h"
//#include "kphoneview.h"

#include "CrossDefine.h" // ПОПОВ CrossDefine callaudio

#ifdef ALSA_SUPPORT
#include "dspoutalsa.h"
#endif


#include "callaudio.h"

CallAudio::CallAudio( QObject* parent ) : QObject(parent)
{
  QSettings settings;
  if(settings.value( "/settings/STUN/UseStun", "No" ).toString() == "Yes")
	setStunSrv(settings.value( "/settings/STUN/StunServer" ).toString());

  _pAudioOutput = 0;
  _pAudioInput = 0;
  _pCurrentSipCall = 0;
  _pCurrentSipCallMember = 0;

  _audioMode = alsa;

  setALSADevicename( QString::null );
  setVideoSW( QString::null );
  _localSDP.setIpAddress( "0.0.0.0" );

  _rtpPayloadSize = 160;// settings.value( "/settings/dsp/SizeOfPayload", 160 ).toInt();
  _audioCodec = codecILBC_20;
  _rtpCodec = codecUnknown;
  _videoCodec = codecUnknown;
  _videoRtpCodec = codecUnknown;
  _bodyMask = QString::null;
  _useStun = false;
  _symMedia = false;

	_incomingThreadTimeUpdateTimer = 0;
  //pidVideo = 0;
  //audio_fd = -1;

  //_pVideoProcess = NULL;
	_pVideo = new VoIPVideo(320, 240, this);
	connect(_pVideo, SIGNAL(pictureShow(const QImage&)), this, SIGNAL(proxyPictureShow(const QImage&)));
	connect(_pVideo, SIGNAL(localPictureShow(const QImage&)), this, SIGNAL(proxyLocalPictureShow(const QImage&)));
	connect(this, SIGNAL(proxyStopCamera()), _pVideo, SLOT(stopCamera()));
	connect(this, SIGNAL(proxyStartCamera()), _pVideo, SLOT(startCamera()));

	//connect(this, SIGNAL(proxySuspendStateChange(bool)), this, SLOT(onProxySuspendStateChange(bool)));
}

CallAudio::~CallAudio( void )
{
	if(_incomingThreadTimeUpdateTimer != 0)
	{
		killTimer(_incomingThreadTimeUpdateTimer);
		_incomingThreadTimeUpdateTimer = 0;
	}


  if( _pAudioOutput )
  {
	if( _pAudioOutput->isRunning() )
	{
	  _pAudioOutput->setCancel();
	  _pAudioOutput->wait();
	}
	delete _pAudioOutput;
  }
  if( _pAudioInput )
  {
	if( _pAudioInput->isRunning() )
	{
	  _pAudioInput->setCancel();
	  _pAudioInput->wait();
	}
	delete _pAudioInput;
  }
	if(_pVideo != NULL)
	{
		delete _pVideo;
		_pVideo = NULL;
	}
}

void CallAudio::timerEvent(QTimerEvent * tEvent)
{
	if(tEvent->timerId() == _incomingThreadTimeUpdateTimer && _pAudioOutput)
	{
		qint64 timeMS = _pAudioOutput->elapsedTime();
		emit incomingThreadTimeChange(timeMS);
	}
}

VoIPVideo* CallAudio::videoControl() const
{
	return _pVideo;
}


void CallAudio::setAudiomode( QString str )
{
  if( str.toUpper() == "ALSA" )
  {
	if( _audioMode == alsa )
	  return;
	_audioMode = alsa;
  }
  renegotiateCall();
}



void CallAudio::setALSADevicename( const QString &devname )
{
  if( devname == QString::null )
  {
	_alsaDeviceName = "default";
  }
  else
  {
	_alsaDeviceName = devname;
  }
}

void CallAudio::setVideoSW( const QString &sw )
{
  if( sw == QString::null )
  {
	_videoSW = "vic";
  }
  else
  {
	_videoSW = sw;
  }
}

void CallAudio::onProxySuspendStateChange(bool state)
{
	if(!state)
	{
		QMessageBox::information(NULL, "CallAudio::onProxySuspendStateChange::false", "........................................");
	}
	else
	{
		QMessageBox::information(NULL, "CallAudio::onProxySuspendStateChange::true", "........................................");
	}
}

void CallAudio::audioIn( void )
{
  QString hostname = _remoteSDP.getIpAddress();
  unsigned int portnum = _remoteSDP.getPort();
  if( hostname == QString::null || portnum == 0 )
  {
	dbgPrintf( QString(tr("CallAudio: SendToRemote called but we don't have a valid session description yet") + "\n").toLocal8Bit().constData() );
	return;
  }
  if( _remoteSDP.isOnHold() )
  {
	dbgPrintf( QString(tr("CallAudio: Remote is currently putting us on hold, waiting patiently") + "\n").toLocal8Bit().constData() );
	return;
  }
  //dbgPrintf( tr("CallAudio: Sending to remote site %s:%d") + "\n", hostname.toLatin1(), portnum );
  dbgPrintf( QString(tr("CallAudio: Sending to remote site %s:%d") + "\n").toLocal8Bit().constData(), hostname.toLocal8Bit().constData(), portnum );

  if( _pAudioInput )
  {
	stopListeningAudio();
  }
  DspOut *out = NULL;
  DspOut *in = NULL;


  if( _audioMode == alsa )
  {
	DspOutRtp *outrtp = NULL;
	if (_symMedia)
	{
	  outrtp = new DspOutRtp( getRtpCodec(), getRtpCodecNum(), hostname, &_symmSocket );
	}
	else
	{
	  outrtp = new DspOutRtp( getRtpCodec(), getRtpCodecNum(), hostname );
	}
	outrtp->setPortNum( portnum );
	outrtp->openDevice( DspOut::WriteOnly );
	outrtp->setPayload( _rtpPayloadSize );

	DspOutAlsa *inaudio = new DspOutAlsa( _alsaDeviceName );
		connect(this, SIGNAL(proxySuspendStateChange(bool)), inaudio, SLOT(onSuspendChanged(bool)));

   // QSettings settings;

	dbgPrintf( "CallAudio: Opening ALSA device for Input \n" );

	if( !inaudio->openDevice( DspOut::ReadOnly ) )
	{
			emit audioInputPresentChange(false);
	  dbgPrintf( "** audioIn: openDevice Failed.\n" );
	}
		else
		{
			emit audioInputPresentChange(true);
		}

	out = outrtp;
	in = inaudio;

	dbgPrintf( "CallAudio: Creating ALSA->RTP Diverter\n" );
	_pAudioInput = new DspAudioIn( in, out );
  }

  // ПОПОВ curcall->getCallType() == SipCall::videoCall
  //if( _pCurrentSipCall->getCallType() == SipCall::videoCall )
  {
	dbgPrintf( QString(tr("CallAudio: Opening SW for video input and output") + "\n").toLocal8Bit().constData() );
	//////////pidVideo = fork();
	//////////if( !pidVideo ) {
	//////////  QSettings settings;
	//////////  QString videoSW = settings.readEntry(
	//////////    "/kphone/video/videoSW", "/usr/local/bin/vic" );
	//////////  QString videoCodec = getVideoRtpCodecName();
	//////////  QString SW = videoSW;
	//////////  if( SW.contains( "/" ) ) {
	//////////    SW = SW.mid( SW.lastIndexOf( '/' ) + 1 );
	//////////  }
	//////////  QString videoSWParam = hostname + "/" +
	//////////    QString::number( remote.getVideoPort() ) + "/";
	//////////  videoSWParam += videoCodec + "/16/" +
	//////////    QString::number( local.getVideoPort() );
	//////////  dbgPrintf( "CallAudio: execlp( %s, %s, %s, 0)\n",
	//////////    videoSW.toLatin1(), SW.toLatin1(), videoSWParam.toLatin1() );
	//////////  execlp( videoSW.toLatin1(), SW.toLatin1(), videoSWParam.toLatin1(), 0 );
	//////////  dbgPrintf( tr("error executing ") + videoSW + "\n" );
	//////////  exit(1);
	//////////}
	//QProcess
  }
  //dbgPrintf( QString(tr("CallAudio: Opening SW for video input and output") + "\n").toLocal8Bit() );
  ////dbgPrintf( QString(tr("CallAudio: Video On ") + hostname + " Port: " + QString::number( remote.getVideoPort() ) + " LocalPort: " + QString::number( local.getVideoPort() ) + "\n").toLocal8Bit() );

  //QString program = "c:/vid/VoIPVideo.exe";
  //QStringList arguments;
  //arguments << "start" << hostname << QString::number( _remoteSDP.getVideoPort() ) << QString::number( _localSDP.getVideoPort() );

  //if(_pVideoProcess == NULL)
  //{
  //  _pVideoProcess = new QProcess(this);
  //  _pVideoProcess->start(program, arguments);
  //}
	if(_pVideo != NULL)
		_pVideo->Set(320, 240, QHostAddress(hostname), _remoteSDP.getVideoPort(), _localSDP.getVideoPort());


  //hostname
  //int videoPort = remote.getVideoPort();
  //int localVideoPort = local.getVideoPort();
}


void CallAudio::outputVideoResolutonChangedToHigh(bool isHigh)
{
	Q_UNUSED(isHigh);
	//qDebug("CallAudio::outputVideoResolutonChangedToHigh(%d)", (int)isHigh);
	//if(_pVideo != NULL)
	//{
	//	if(isHigh)
	//	{
	//		//_pVideo->SetFrameSize(640, 480);
	//		_pVideo->SetFrameSize(560, 420);
	//	}
	//	else
	//	{
	//		_pVideo->SetFrameSize(320, 240);
	//	}
	//}
}



void CallAudio::stopListeningAudio( void )
{
  if( _pAudioInput )
  {
	if( _pAudioInput->isRunning() )
	{
	  _pAudioInput->setCancel();
	  _pAudioInput->wait();
	}
	delete _pAudioInput;
  }
  _pAudioInput = 0;

  _remoteSDP = SdpMessage::null;
}



// Вывод звука
SdpMessage CallAudio::audioOut( void )
{
  dbgPrintf( "CallAudio: listening for incomming RTP\n" );
  if( _pAudioOutput )
  {
	// ВНИМАНИЕ: REINVITE RE_INVITE ПРОВЕРИТЬ Аудио поток при изменениях
	return _localSDP;
	//stopSendingAudio();
  }
  DspOut *out = NULL; // Исходящий поток (Уходит на устройство вывода звука)
  DspOut *in = NULL;  // Входящий поток (принимаемый RTP)

  // Адрес локальной машины и наименование RTP потока
  _localSDP.setIpAddress( Sip::getLocalAddress() );
  _localSDP.setName( "Rambler VoIP RTP" );

  if( _audioMode == alsa ) // Модель alsa Linux
  {
	// **** INPUT! MEDIA FROM NET ****
	// Создаем входящий поток RTP
	DspOutRtp *inrtp = NULL;
	if (_symMedia)
	{
	  inrtp = new DspOutRtp( getRtpCodec(), getRtpCodecNum(), QString::null, &_symmSocket );
	}
	else
	{
	  inrtp = new DspOutRtp( getRtpCodec(), getRtpCodecNum() );
	}
	inrtp->setPayload( _rtpPayloadSize ); // Полезная нагрузка 160
	if( _useStun ) // Использование stun
	{
	  inrtp->setStunSrv( _stunSrv );
	}
	inrtp->openDevice( DspOut::ReadOnly ); // RTP только читаем (поток вывода звука)
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// Из RTP сохраняем в SDP порты для приема звука и видео потоков
	_localSDP.setPort( inrtp->getPortNum() ); // Устанавливаем порты
	_localSDP.setVideoPort( inrtp->getVideoPortNum() );

	// **** OUTPUT! MEDIA TO OUTPUT MEDIA DEVICE *****
	// Выход через alsa (осталось из linux). В данном случае вывод через систему мультимедия QT)
	DspOutAlsa *outalsa = new DspOutAlsa( _alsaDeviceName );
		//connect(this, SIGNAL(proxySuspendStateChange(bool)), outalsa, SLOT(onSuspendChanged(bool)));


	//QSettings settings; // ПОПОВ НАКОЙ тут settings???
	dbgPrintf( "CallAudio: Opening ALSA device for Output\n" );

	// Выводящий девайс открываем на чтение (актуально для Linux)
	if( !outalsa->openDevice( DspOut::WriteOnly ))
	{
	  dbgPrintf( "** audioOut: openDevice Failed.\n" );
			emit audioOutputPresentChange(false);
	}
		else
		{
			emit audioOutputPresentChange(true);
		}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


	// **** СУММАРНАЯ ИНИЦИАЛИЗАЦИЯ ЗВУКОВОГО ОБЪЕКТА ****
	in = inrtp;    // Ощая инициализация вход: RTP
	out = outalsa; // Ощая инициализация выход: Device
	dbgPrintf( QString(tr("CallAudio: Creating RTP->ALSA Diverter") + "\n").toLocal8Bit().constData() );
	_pAudioOutput = new DspAudioOut( in, out );

		//connect(_pAudioOutput, SIGNAL(finished()), this, SIGNAL(outputDead()));
		//connect(_pAudioOutput, SIGNAL(terminated()), this, SIGNAL(outputDead()));

		//connect(_pAudioOutput, SIGNAL(terminated()), this, SLOT(onTerminated()));
		//connect(_pAudioOutput, SIGNAL(finished()), this, SLOT(onFinished()));
		//connect(_pAudioOutput, SIGNAL(terminated()), this, SLOT(onUserExit()));
  }

  return _localSDP;
}

void CallAudio::onTerminated()
{
	//qDebug("CallAudio::onTerminated");
}
void CallAudio::onFinished()
{
	qDebug("CallAudio::onFinished");
	emit outputDead();
}
void CallAudio::onUserExit()
{
	//qDebug("CallAudio::onUserExit");
}


void CallAudio::stopSendingAudio( void )
{
  if( _pAudioOutput )
  {
	if( _pAudioOutput->isRunning() )
	{
	  _pAudioOutput->setCancel();
	  _pAudioOutput->wait();

			if(_incomingThreadTimeUpdateTimer != 0)
				killTimer(_incomingThreadTimeUpdateTimer);
	}
	delete _pAudioOutput;
  }
  _pAudioOutput = 0;

  _localSDP.setIpAddress( "0.0.0.0" );
  _localSDP.setName( "Idle" );
  _localSDP.setPort( 0 );
  _localSDP.setVideoPort( 0 );
}

void CallAudio::saveAudioSettings( void )
{
  //QSettings settings;
  //settings.writeEntry( "/kphone/audio/oss-filename", getOSSFilename() );
  //settings.writeEntry( "/kphone/audio/oss-filename2", getOSSFilename2() );
}

void CallAudio::readAudioSettings( void )
{
  //QSettings settings;
  //setALSADevicename( settings.value( "/settings/audio/alsa-devicename" ).toString() );
  //if( settings.value( "/settings/Symmetric/Media", "Yes" ).toString() == "Yes" )
  //{
  //  setSymMediaMode( true );
  //}
	// ВНИМАНИЕ !!! ???
	setALSADevicename("");
	setSymMediaMode( true );

  QString audiomodestr = "alsa";// = settings.value( "/settings/audio/audio", "alsa" ).toString();
  setAudiomode( audiomodestr );

  //QString codec = settings.value( "/settings/audio/Codec", "PCMU" ).toString();
  //if( codec == "PCMU" )
  //{
  //  setCodec( codecPCMU );
  //}
  //else if( codec == "GSM" )
  //{
  //  setCodec( codecGSM );
  //}
  //else if( codec == "PCMA" )
  //{
  //  setCodec( codecPCMA );
  //}
  //else if( codec == "ILBC_20" )
  //{
  //  setCodec( codecILBC_20 );
  //}
  //else if( codec == "ILBC" )
  //{
  //  setCodec( codecILBC_30 );
  //}
  //else if( codec == "SPEEX")
  //{
  //  setCodec( codecSpeex );
  //}

	// ВНИМАНИЕ! установка кодека set codec
	//setCodec( codecPCMU );
	//setCodec( codecPCMA );
	//setCodec( codecILBC_20 );
	setCodec( codecSpeex );
}

void CallAudio::readVideoSettings( void )
{
#pragma message("CallAudio::readVideoSettings разобраться!!!")
  //QSettings settings;
  //setVideoSW( settings.value( "/settings/video/videoSW", "/usr/local/bin/vic"  ).toString() );
  //QString videoCodec = settings.value( "/settings/video/codec", "h264" ).toString();
  //if( videoCodec == "h261" )
  //{
  //  setVideoCodec( codecH261 );
  //}
  //else if( videoCodec == "h263" )
  //{
  //  setVideoCodec( codecH263 );
  //}
  //else if( videoCodec == "h264" )
  //{
  //  setVideoCodec( codecH264 );
  //}
	setVideoSW("VoIPVideo");
	setVideoCodec( codecH264 );
}

void CallAudio::setCurrentCall( SipCall *newcall )
{
  _pCurrentSipCall = newcall;
  emit statusUpdated();
}

void CallAudio::attachToCallMember( SipCallMember *newmember )
{
  if( _pCurrentSipCallMember )
  {
	detachAndHold();
  }
  _pCurrentSipCallMember = newmember;
  connect( _pCurrentSipCallMember, SIGNAL( statusUpdated(SipCallMember *) ), this, SLOT( memberStatusUpdated(SipCallMember *) ) );
  statusUpdated();
}

void CallAudio::detachAndHold( void )
{
  if( _localSDP.isOnHold() )
  {
	dbgPrintf( "CallAudio: Call already on hold\n" );
  }
  else
  {
	toggleOnHold();
  }
}

void CallAudio::toggleOnHold( void )
{
  if( _localSDP.isOnHold() )
  {
	dbgPrintf( "CallAudio: Resuming call\n" );
	_pCurrentSipCallMember->requestInvite( audioOut().message( getRtpCodec(), getVideoRtpCodec(), getBodyMask() ), MimeContentType( "application/sdp" ) );
  }
  else
  {
	dbgPrintf( "CallAudio: Putting call on hold\n" );
	_localSDP.setIpAddress( "0.0.0.0" );
	_localSDP.setName( "Whoa there dewd" );
	_localSDP.setPort( 0 );
	_localSDP.setVideoPort( 0 );
	_pCurrentSipCallMember->requestInvite( _localSDP.message( getRtpCodec(), getVideoRtpCodec(), getBodyMask() ), MimeContentType( "application/sdp" ) );
	detachFromCall();
  }
}

bool CallAudio::checkCodec( SipCallMember *member )
{
  bool status = true;
  QString mstr = member->getSessionDescription();
  _rtpCodec = codecUnknown;
  _videoRtpCodec = codecUnknown;
  if( mstr.contains( "m=audio" ) )
  {
	//QStringList mstrList = mstr.split('\n', QString::SkipEmptyParts, Qt::CaseInsensitive);

	QString ilbc = "";
	QString speex = "";
	QString m = mstr.mid( mstr.indexOf( "m=audio" ) );
	m = m.left( m.indexOf( "\n" ) );
	m = m.mid( m.indexOf( "RTP/AVP" ) + 7 );
	m += ' ';

	// dynamic port 96-127
	if( mstr.toLower().contains( "ilbc/8000" ) )
	{
	  ilbc = mstr.mid( mstr.toLower().indexOf( "ilbc/8000" ) - 7, 6 );
	  if( ilbc.contains( ":" ) )
	  {
	ilbc = ilbc.mid( ilbc.indexOf( ":" ) + 1 );
	  }
	  ilbc = ilbc.simplified();
	}

	// ПОПОВ Добавил speex
	if( mstr.toLower().contains( "speex/8000" ) )
	{
	  speex = mstr.mid( mstr.toLower().indexOf( "speex/8000" ) - 7, 6 );
	  if( speex.contains( ":" ) )
	  {
	speex = speex.mid( speex.indexOf( ":" ) + 1 );
	  }
	  speex = speex.simplified();
	}

	int posPCMU = m.indexOf( " 0 " );
	int posGSM  = m.indexOf( " 3 " );
	int posPCMA = m.indexOf( " 8 " );
	int posILBC = (ilbc.isEmpty()) ? -1 : m.toLower().indexOf( " " + ilbc + " " );
	int posSpeex = (speex.isEmpty()) ? -1 : m.toLower().indexOf( " " + speex + " " );
	if( posPCMU < 0 ) posPCMU = 101;
	if( posGSM  < 0 ) posGSM  = 101;
	if( posPCMA < 0 ) posPCMA = 101;
	if( posILBC < 0 ) posILBC = 101;
	if( posSpeex < 0 ) posSpeex = 101;

#define MIN(a,b) (a)<(b)?(a):(b)

	int winner = MIN ( posPCMU , posGSM );
	winner = MIN ( winner , posPCMA );
	winner = MIN ( winner , posILBC );
	winner = MIN ( winner , posSpeex );

	if ( winner == posPCMU )
	{
	  _rtpCodec = codecPCMU;
	  _rtpCodecNum = 0;
	}
	else
	  if ( winner == posGSM )
	  {
	_rtpCodec = codecGSM;
	_rtpCodecNum = 3;
	  }
	  if ( winner == posPCMA )
	  {
	_rtpCodec = codecPCMA;
	_rtpCodecNum = 8;
	  }
	  else
	if( winner == posILBC )
	{
	  _rtpCodec = codecILBC_30;
	  _rtpCodecNum = ilbc.toInt();
	  if( mstr.contains( "a=fmtp:" ) )
	  {
		QString m = mstr.mid( mstr.indexOf( "a=fmtp:" ) );
		m = m.left( m.indexOf( "\n" ) );
		if( m.toLower().contains( "mode=20" ) )
		{
		  _rtpCodec = codecILBC_20;
		}
	  }
	}
	if( winner == posSpeex ) // ПОПОВ Speex
	{
	  _rtpCodec = codecSpeex;
	  _rtpCodecNum = speex.toInt();
	}
  }
  if( mstr.contains( "m=video" ) )
  {
	QString ilbc = "";
	QString m = mstr.mid( mstr.indexOf( "m=video" ) );
	m = m.left( m.indexOf( "\n" ) );
	m = m.mid( m.indexOf( "RTP/AVP" ) + 7 );
	m += ' ';
	int posH261 = m.indexOf( " 31 " );
	int posH263 = m.indexOf( " 34 " );
	int posH264 = m.indexOf( " 99 " );
	if( posH261 < 0 ) posH261 = 101;
	if( posH263 < 0 ) posH263 = 101;
	if( posH264 < 0 ) posH264 = 101;
#ifdef MESSENGER
	if( _videoCodec == codecH261 )
	{
	  _videoRtpCodec = codecH261;
	  _videoRtpCodecNum = 31;
	}
	else
	{
	  _videoRtpCodec = codecH263;
	  _videoRtpCodecNum = 34;
	}
#else

	int winner = MIN ( posH261 , posH263 );
	winner = MIN ( winner , posH264 );

	if(winner == posH261)
	{
	  _videoRtpCodec = codecH261;
	  _videoRtpCodecNum = 31;
	}
	else if(winner == posH263)
	{
	  _videoRtpCodec = codecH263;
	  _videoRtpCodecNum = 34;
	}
	else if(winner == posH264)
	{
	  _videoRtpCodec = codecH264;
	  _videoRtpCodecNum = 99;
	}

	//////////if( posH261 < posH263 )
	//////////{
	//////////  _videoRtpCodec = codecH261;
	//////////  _videoRtpCodecNum = 31;
	//////////}
	//////////else if( posH263 < posH261 )
	//////////{
	//////////  _videoRtpCodec = codecH263;
	//////////  _videoRtpCodecNum = 34;
	//////////}
#endif
  }

  if( _rtpCodec == codecILBC_20 )
  {
	dbgPrintf( "CallAudio: Using iLBC 20ms for output\n" );
  }
  else if( _rtpCodec == codecILBC_30 )
  {
	dbgPrintf( "CallAudio: Using iLBC 30ms for output\n" );
  }
  else if( _rtpCodec == codecPCMA )
  {
	dbgPrintf( "CallAudio: Using G711a for output\n" );
  }
  else if( _rtpCodec == codecGSM )
  {
	dbgPrintf( "CallAudio: Using GSM for output\n" );
  }
  else if( _rtpCodec == codecPCMU )
  {
	dbgPrintf( "CallAudio: Using G711u for output\n" );
  }
  else if( _rtpCodec == codecSpeex )
  {
	dbgPrintf( "CallAudio: Using SPEEX for output\n" );
  }
  else if( _rtpCodec == codecUnknown )
  {
	status = false;
  }
  if( _rtpCodec != codecUnknown )
  {
	if( _videoRtpCodec == codecH261 )
	{
	  dbgPrintf( "CallAudio: Using H261 for video output\n" );
	}
	else if( _videoRtpCodec == codecH263 )
	{
	  dbgPrintf( "CallAudio: Using H263 for video output\n" );
	}
	else if( _videoRtpCodec == codecH264 )
	{
	  dbgPrintf( "CallAudio: Using H264 for video output\n" );
	}
	else
	{
	  if( _pCurrentSipCall )
	  {
	_pCurrentSipCall->setCallType( SipCall::StandardCall );
	  }
	}
  }

  return status;
}

void CallAudio::memberStatusUpdated(SipCallMember *member)
{
  SdpMessage sdpm;
  SdpMessage rsdp;
  _pCurrentSipCallMember = member;
  sdpm.parseInput( _pCurrentSipCallMember->getSessionDescription() );
  if( checkCodec( _pCurrentSipCallMember ) )
  {
	if( _pCurrentSipCallMember->getState() == SipCallMember::state_RequestingReInvite )
	{
	  if( sdpm.isOnHold() )
	  {
	rsdp.setName( "Accepting on hold" );
	rsdp.setIpAddress( "0.0.0.0" );
	rsdp.setPort( 0 );
	rsdp.setVideoPort( 0 );
	_pCurrentSipCallMember->acceptInvite( rsdp.message( getRtpCodec(), getVideoRtpCodec(), getBodyMask() ), MimeContentType( "application/sdp" ) );
	stopSendingAudio();
	stopListeningAudio();
	  }
	  else
	  {
	_pCurrentSipCallMember->acceptInvite( audioOut().message( getRtpCodec(), getVideoRtpCodec(), getBodyMask() ), MimeContentType( "application/sdp" ) );
	  }
	}
	if( sdpm != _remoteSDP )
	{
	  _remoteSDP = sdpm;
	  if( !sdpm.isOnHold() )
	  {
	if( _pAudioOutput )
	{
	  _pAudioOutput->setCodec( getRtpCodec(), getRtpCodecNum() );
	  audioIn();
	  _pAudioOutput->start();
					_incomingThreadTimeUpdateTimer = startTimer(1000);

	  _pAudioInput->start();
	}
	////else if( jack_audioout )
	////{
	////  //					jack_audioout->setCodec( getRtpCodec(), getRtpCodecNum() );
	////  audioIn();
	////  /*
	////  jack_audioout->start();
	////  jack_audioin->start();
	////  */
	////}
	  }
	  statusUpdated();
	}
  }
}

void CallAudio::detachFromCall( void )
{
  if( _pCurrentSipCallMember )
  {
	disconnect( _pCurrentSipCallMember, 0, this, 0 );
  }
  _pCurrentSipCallMember = 0;
  stopSendingAudio();
  stopListeningAudio();
  setCurrentCall( 0 );

  //if(_pVideoProcess)
  //{
  //  _pVideoProcess->terminate();
  //  delete _pVideoProcess;
  //  _pVideoProcess = NULL;
  //}

	_pVideo->Stop();
  // ПОПОВ if( pidVideo )
  //if( pidVideo )
  //{
  //  kill( pidVideo, SIGKILL );
  //  pidVideo = 0;
  //}
}

bool CallAudio::isRemoteHold( void )
{
  return _remoteSDP.isOnHold();
}

void CallAudio::renegotiateCall( void )
{
  if( !_pCurrentSipCall ) return;
  stopSendingAudio();
  stopListeningAudio();
  _pCurrentSipCallMember->requestInvite( audioOut().message( getRtpCodec(), getVideoRtpCodec(), getBodyMask() ), MimeContentType( "application/sdp" ) );
}

bool CallAudio::isAudioOn( void )
{
  return (_pAudioOutput || _pAudioInput );
}

codecType CallAudio::getRtpCodec( void )
{
  if( _rtpCodec != codecUnknown )
  {
	return _rtpCodec;
  }
  else
  {
	return _audioCodec;
  }
}

int CallAudio::getRtpCodecNum( void )
{
  int c;
  if( _rtpCodec != codecUnknown )
  {
	c = _rtpCodecNum;
  }
  else
  {
	switch( _audioCodec )
	{
	  case codecGSM:
	c = 3;
	break;
	  case codecPCMA:
	c = 8;
	break;
	  case codecILBC_20:
	  case codecILBC_30:
	c = 98;
	break;
	  case codecSpeex:
	c = 97;
	break;
	  case codecPCMU:
	  default:
	c = 0;
	break;
	}
  }
  return c;
}

codecType CallAudio::getVideoRtpCodec( void )
{
  if( _videoRtpCodec != codecUnknown )
  {
	return _videoRtpCodec;
  }
  else
  {
	if( _pCurrentSipCall )
	{
	  if( _pCurrentSipCall->getCallType() == SipCall::videoCall )
	  {
	return _videoCodec;
	  }
	  else
	  {
	return codecUnknown;
	  }
	}
	else
	{
	  return codecUnknown;
	}
  }
}

int CallAudio::getVideoRtpCodecNum( void )
{
  int c;
  if( _pCurrentSipCall )
  {
	if( _pCurrentSipCall->getCallType() != SipCall::videoCall )
	{
	  return -1;
	}
  }
  if( _videoRtpCodec != codecUnknown )
  {
	c = _videoRtpCodecNum;
  }
  else
  {
	switch( _videoCodec )
	{
	  case codecH261:
	c = 31;
	break;
	  case codecH263:
	c = 34;
	break;
	  case codecH264:
	c = 99;
	break;
	  default:
	c = -1;
	break;
	}
  }
  return c;
}

QString CallAudio::getVideoRtpCodecName( void )
{
  QString c;
  if( _pCurrentSipCall->getCallType() != SipCall::videoCall )
  {
	return "";
  }
  if( _videoRtpCodec != codecUnknown )
  {
	switch( _videoRtpCodec )
	{
	  case codecH263:
	c = "h263";
	break;
	  default:
	  case codecH261:
	c = "h261";
	break;
	  case codecH264:
	c = "h264";
	break;
	}
  }
  else
  {
	switch( _videoCodec )
	{
	  case codecH263:
	c = "h263";
	break;
	  default:
	  case codecH261:
	c = "h261";
	break;
	  case codecH264:
	c = "h264";
	break;
	}
  }
  return c;
}

void CallAudio::setStunSrv( QString newStunSrv )
{
  _useStun = true;
  _stunSrv = newStunSrv;
}

void CallAudio::startDTMF(char code)
{
  if (_pAudioOutput)
  {
	_pAudioOutput->startTone(code);
  }

  if (_pAudioInput)
  {
	_pAudioInput->startTone(code);
  }
}

void CallAudio::stopDTMF(void)
{
  if (_pAudioOutput)
  {
	_pAudioOutput->stopTone();
  }

  if (_pAudioInput)
  {
	_pAudioInput->stopTone();
  }
}


