#ifndef VIDEOLAYOUT_H
#define VIDEOLAYOUT_H

#include <QRectF>
#include <QLayout>
#include <QWidget>
#include <QPropertyAnimation>
#include "videoframe.h"

class SimpleAnimation : 
	public QVariantAnimation
{
	Q_OBJECT
protected:
	virtual void updateCurrentValue(const QVariant &AValue) { Q_UNUSED(AValue); }
};

class VideoLayout : 
	public QLayout
{
	Q_OBJECT
public:
	VideoLayout(VideoFrame *ARemoteVideo, VideoFrame *ALocalVideo, QWidget *AButtons, QWidget *AParent);
	~VideoLayout();
	// QLayout
	int count() const;
	void addItem(QLayoutItem *AItem);
	QLayoutItem *itemAt(int AIndex) const;
	QLayoutItem *takeAt(int AIndex);
	// QLayoutItem
	QSize sizeHint() const;
	void setGeometry(const QRect &ARect);
	// VideoLayout
	int locaVideoMargin() const;
	void setLocalVideoMargin(int AMargin);
	bool isVideoVisible() const;
	void setVideoVisible(bool AVisible);
	void setControlsWidget(QWidget *AControls);
	bool isControlsVisible() const;
	void setControlsVisible(bool AVisible);
public slots:
	void saveLocalVideoGeometry();
	void restoreLocalVideoGeometry();
protected:
	void saveLocalVideoGeometryScale();
	Qt::Alignment remoteVideoAlignment() const;
	Qt::Alignment geometryAlignment(const QRect &AGeometry) const;
	QRect adjustRemoteVideoPosition(const QRect &AGeometry) const;
	QRect adjustLocalVideoSize(const QRect &AGeometry) const;
	QRect adjustLocalVideoPosition(const QRect &AGeometry) const;
	QRect correctLocalVideoPosition(const QRect &AGeometry) const;
	QRect correctLocalVideoSize(Qt::Corner ACorner, const QRect &AGeometry) const;
protected slots:
	void onLocalVideoStateChanged();
	void onLocalVideoSingleClicked();
	void onLocalVideoDoubleClicked();
	void onLocalVideoMove(const QPoint &APos);
	void onLocalVideoResize(Qt::Corner ACorner, const QPoint &APos);
	void onControlsVisibilityPercentChanged(const QVariant &AValue);
private:
	bool FCtrlVisible;
	bool FVideoVisible;
	int FLocalMargin;
	int FLocalStickDelta;
	double FCtrlVisiblePerc;
	QRectF FLocalScale;
	QWidget *FButtons;
	QWidget *FControls;
	VideoFrame *FLocalVideo;
	VideoFrame *FRemoteVideo;
	SimpleAnimation FCtrlAnimation;
};

#endif // VIDEOLAYOUT_H
