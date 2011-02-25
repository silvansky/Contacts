#ifndef DSPAUDIOOUT_H_INCLUDED
#define DSPAUDIOOUT_H_INCLUDED

#include "voipmedia_global.h"

#include "../config.h"
#include "dspaudio.h"

#include <QFile>

class DspOut;

// Класс вывода звука
class VOIPMEDIA_EXPORT DspAudioOut : public DspAudio
{
#ifndef QT_THREAD_SUPPORT
  Q_OBJECT
#endif
public:
  DspAudioOut( DspOut *in, DspOut *out );
  ~DspAudioOut( void );

  /**
  * Just do one tick, for non-threaded implementations.
  */
  virtual void timerTick( void );

#ifndef QT_THREAD_SUPPORT
  virtual void start( void );

  private slots:
    virtual void timeout( void );

#endif

QFile tempFileToRtp;


private:
  unsigned int curpos;
};

#endif // DSPAUDIOOUT_H_INCLUDED
