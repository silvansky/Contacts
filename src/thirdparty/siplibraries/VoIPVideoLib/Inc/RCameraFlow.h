#ifndef RCAMERAFLOW_H
#define RCAMERAFLOW_H

#include <QObject>
#include <QImage>
#include <QTimerEvent>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>

class videoInput;

class RCameraFlow : public QObject
{
  Q_OBJECT

public:
  RCameraFlow(QObject *parent);
  ~RCameraFlow();

public:
  QImage currentFrame() const;
  bool isOk() const;
	bool cameraPresent() const;
	QSize frameSize() const;

	// Попытка инициализировать камеру с заданным размером кадра.
	// posibleSize - в случае если заданный размер кадра не поддерживается, возвращается возможный размер кадра
	bool tryThisFrameSize(int width, int height, QSize& posibleSize);

	// Устанавливаем максимально-возможный размер картинки для камеры
	QSize setMaxFrameSize();

public slots:
	bool stop();
	bool start();

protected:
  void timerEvent(QTimerEvent*);

private:
  //void toQImage(IplImage *cvimage);
  void toQImage(uchar* pixels, int width, int height);

private:
	int _numDevices;
  int _flowTimer;
  bool _cameraStart;
  //CvCapture *_camera;
  QImage _frame;
	QSize _frameSize;

  videoInput* pVInput;
};

#endif // RCAMERAFLOW_H
