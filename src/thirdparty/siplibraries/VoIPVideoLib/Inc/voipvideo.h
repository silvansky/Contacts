#ifndef VOIPVIDEO_H
#define VOIPVIDEO_H

#include <QObject>
#include "videotranslator.h"
#include "voipvideolib_global.h"

class QHostAddress;
class QImage;



class VOIPVIDEOLIB_EXPORT VoIPVideo : public QObject
{
	Q_OBJECT

public:
	VoIPVideo(int width, int height, const QHostAddress& remoteHost, int remoteVideoPort, int localVideoPort, QObject *parent);
	VoIPVideo(int width, int height, QObject *parent = NULL);
	~VoIPVideo();

	void Set(int width, int height, const QHostAddress& remoteHost, int remoteVideoPort, int localVideoPort);
	void SetFrameSize(int width, int height);
	void Stop();
	bool checkCameraPresent() const;

public slots:
	void stopCamera();
	void startCamera();

signals:
	void localPictureShow(const QImage&);
	void pictureShow(const QImage&);
	void sysInfo(const QString&);
	
private:
	VideoTranslator* _vTrans;
};

#endif // VOIPVIDEO_H
