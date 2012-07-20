#ifndef DSPTHREAD_H_INCLUDED
#define DSPTHREAD_H_INCLUDED

#include "voipmedia_global.h"

#include "../config.h"

#ifdef QT_THREAD_SUPPORT
#include <qthread.h>
#else
#include <qtimer.h>
#include <qobject.h>
#endif

#include "sdp.h"
#include "audiobuffer.h"
#include "dtmfgenerator.h"


class DspOut;

#ifdef QT_THREAD_SUPPORT
class VOIPMEDIA_EXPORT DspAudio : public QThread
#else
class VOIPMEDIA_EXPORT DspAudio : public QObject
#endif
{
public:
  DspAudio( DspOut *in, DspOut *out );
  virtual ~DspAudio( void );

#ifdef QT_THREAD_SUPPORT
  virtual void run();
#else
  virtual void start( void ) = 0;
  bool running( void ) { return false; }
  void wait( void ) { return; }
  void exit( void ) { return; }
#endif

  virtual void timerTick( void ) = 0;
  void startTone(char code);
  void stopTone(void);

  /*
  * Generate DTMF if required
  * returns 1 if DTMF was generated
  * and 0 if not
  */
  bool generateDTMF(short* buffer, size_t n);
  void setCancel( void ) { _cancel = true; }
  void setCodec( const codecType newCodec, int newCodecNum );

protected:
  bool isCanceled( void ) { return _cancel; }
  DspOut *_input;
  DspOut *_output;
  bool _broken;
  bool _cancel;
  
  DTMFGenerator _dtmf; // ПОПОВ Исключил DTMFGenerator. Надо делать свой!
  AudioBuffer _copybuffer;
  unsigned int _dtmfSamples;
  char _newTone;      // New DTMF tone to be generated (0 if none)
  char _currentTone;  // Tone currently being generated (0 if none)

  int delay;

public:
	qint64 elapsedTime() const;
private:
	qint64 _startTimeMsecs;
	qint64 _currTimeMsecs;


#ifndef QT_THREAD_SUPPORT
  QTimer *_timer;
#endif

};

#endif // DSPTHREAD_H_INCLUDED
