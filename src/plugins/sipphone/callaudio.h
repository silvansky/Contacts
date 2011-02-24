#ifndef CALLAUDIO_H_INCLUDED
#define CALLAUDIO_H_INCLUDED

#include <qobject.h>
#include <qstring.h>
#include <sys/types.h>

#include <QProcess>

// Протокол SIP
#include <sdp.h>
#include <udpmessagesocket.h>
#include "VoIPVideo.h"
//#include <QImage>

class DspAudio;
class SipCall;
class SipCallMember;
//class KPhoneView;

enum AudioModeType
{
  alsa
};


class CallAudio : public QObject
{
  Q_OBJECT
public:
  CallAudio( QObject* parent = 0 );
  ~CallAudio( void );
  void setAudiomode( QString str );
  ////void setOSSFilename( const QString &devname );
  ////void setOSSFilename2( const QString &devname );
  ////const QString &getOSSFilename( void ) const { return ossfilename; }
  ////const QString &getOSSFilename2( void ) const { return ossfilename2; }
  void setALSADevicename( const QString &devname );
  void setVideoSW( const QString &sw );
  const QString &getVideoSW( void ) const { return _videoSW; }

  // Call attachment
  void attachToCallMember( SipCallMember *newmember );
  void detachFromCall( void );
  void toggleOnHold( void );
  void renegotiateCall( void );

  void startDTMF(char code); // Start DTMF tone generation
  void stopDTMF(void);  // Stop DTMF tone generation

  // Returns a session description for sending in responses
  SdpMessage audioOut( void );

  // Call tracking
  void setCurrentCall( SipCall *newcall );
  SipCall *getCurrentCall( void ) const { return _pCurrentSipCall; }

  bool isRemoteHold( void );

  // Settings
  void saveAudioSettings( void );
  void readAudioSettings( void );
  void readVideoSettings( void );
  void setPayload( int newPayload ) { _rtpPayloadSize = newPayload; }
  bool isAudioOn( void );

  // Set/reset symmetric mode
  void setSymMediaMode( bool yesno ) { _symMedia = yesno; }

  void setCodec( codecType newCodec ) { _audioCodec = newCodec; }
  void setRtpCodec( codecType newCodec ) { _rtpCodec = newCodec; }
  void setVideoCodec( codecType newCodec ) { _videoCodec = newCodec; }
  void setVideoRtpCodec( codecType newCodec ) { _videoRtpCodec = newCodec; }
  codecType getRtpCodec( void );
  int getRtpCodecNum( void );
  short getRtpBlockLength( void );
  codecType getVideoRtpCodec( void );
  int getVideoRtpCodecNum( void );
  QString getVideoRtpCodecName( void );
  void setBodyMask( QString body ) { _bodyMask = body; }
  QString getBodyMask( void ) { return _bodyMask; }
  bool checkCodec( SipCallMember *member );
  void setStunSrv( QString newStunSrv );

signals:
  void outputDead( void );
  void statusUpdated( void );
	void proxyPictureShow(const QImage&);
	void proxyLocalPictureShow(const QImage&);

private slots:
  void memberStatusUpdated(SipCallMember *member);


private:
  void audioIn( void );
  void stopListeningAudio( void );
  void stopSendingAudio( void );
  void detachAndHold( void );

private:
  //KPhoneView *view;
  DspAudio *_pAudioInput;
  DspAudio *_pAudioOutput;
  AudioModeType _audioMode;

  ////DspAudioJackOut *jack_audioout;
  ////DspAudioJackIn  *jack_audioin;
  //int audio_fd;

  SdpMessage _localSDP;
  SdpMessage _remoteSDP;
  ////QString ossfilename;
  ////QString ossfilename2;
  QString _alsaDeviceName; //Added by bobosch
  QString _videoSW;
  SipCall *_pCurrentSipCall;
  SipCallMember *_pCurrentSipCallMember;

  int _rtpPayloadSize;
  codecType _audioCodec;
  codecType _rtpCodec;
  int _rtpCodecNum;

  codecType _videoCodec;
  codecType _videoRtpCodec;
  int _videoRtpCodecNum;

  QString _bodyMask;
  //pid_t pidVideo;
  bool _useStun;
  bool _symMedia;
  QString _stunSrv;

  //QProcess *_pVideoProcess;
	VoIPVideo* _pVideo;

  /*
  * Common socket for both streams, used only in symmetric mode
  */
  UDPMessageSocket _symmSocket;
};


#endif // CALLAUDIO_H_INCLUDED
