#ifndef DSPOUTALSA_H_INCLUDED
#define DSPOUTALSA_H_INCLUDED

#include "voipmedia_global.h"

//#include <alsa/asoundlib.h>
#include "dspout.h"

//#include <Phonon>
//#include <Phonon/MediaObject>
//#include <Phonon/AudioOutput>
//#include <Phonon/ObjectDescription>
#include <QBuffer>
#include <QFile>



#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioInput>
#include <QAudioDeviceInfo>


class VOIPMEDIA_EXPORT DspOutAlsa : public DspOut
{
  Q_OBJECT

public:
  /**
  * Constructs a DspOutAlsa object representing the given
  * filename.  Default is default.
  */
  DspOutAlsa( const QString &fileName = "default" ); //Changed by bobosch

  /**
  * Destructor.  Will close the device if it is open.
  */
  virtual ~DspOutAlsa( void );

  bool openDevice( DeviceMode mode );
  bool writeBuffer( void );
  unsigned int readableBytes( void );
  bool readBuffer( int bytes );
  //int audio_fd;

	// готово ли соответствующее устройство?
	bool inputDeviceReady() const { return _inputReady; }
	bool outputDeviceReady() const { return _outputReady; }

public slots:
  void onSuspendChanged(bool);
	void onInputStateChanged(QAudio::State);

signals:
	void outputPresentChange(bool);
	void inputPresentChange(bool);

private:
  //int err;

  //QString devname;         // device filename

  //QAudioFormat      settings;

  // Вывод мультимедиа потока
  QAudioOutput* _pAudioOutput; // class member.
  QBuffer* _pOutputBuffer;

  // Ввод мултимедиа потока
  QAudioInput* _pAudioInput; // class member.
  QBuffer* _pInputBuffer;

  QFile tempFileToRtp;

  QBuffer* tmpBuffer;

	bool _inputReady;
	bool _outputReady;

  //QFile outFile;
};

#endif  // DSPOUTOSS_H_INCLUDED
