#ifndef DSPAUDIOIN_H_INCLUDED
#define DSPAUDIOIN_H_INCLUDED

#include "voipmedia_global.h"

#include "dspaudio.h"

#include "../config.h"

#include <QFile>

class DspOut;

class VOIPMEDIA_EXPORT DspAudioIn : public DspAudio
{
#ifndef QT_THREAD_SUPPORT
  Q_OBJECT
#endif
public:
  DspAudioIn( DspOut *in, DspOut *out );
  ~DspAudioIn( void );

  /**
  * Just do one tick, for non-threaded implementations.
  */
  virtual void timerTick( void );

  

#ifndef QT_THREAD_SUPPORT
  virtual void start( void );

  private slots:
    virtual void timeout( void );

#endif


  //QFile tempFile;

private:
  int udp_failures;
};

#endif // DSPAUDIOIN_H_INCLUDED
