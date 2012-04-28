#ifndef VOLUMECONTROL_H
#define VOLUMECONTROL_H

#include <QWidget>

class VolumeControl : 
	public QWidget
{
	Q_OBJECT;
public:
	VolumeControl(QWidget *AParent);
	~VolumeControl();
	qreal volume() const;
	void setVolume(qreal AVolume);
	qreal maximumValume() const;
	void setMaximumValume(qreal AVolume);
signals:
	void volumeChanged(qreal AVolume);
	void maximumVolumeChanged(qreal AVolume);
protected:
	void updatePixmap();
	qreal positionToVolume(const QPoint &APos) const;
protected:
	QSize sizeHint() const;
	void paintEvent(QPaintEvent *AEvent);
	void wheelEvent(QWheelEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
private:
	qreal FVolume;
	qreal FMaximumVolume;
private:
	bool FPressed;
	QSize FSizeHint;
	QPixmap FCurPixmap;
};

#endif // VOLUMECONTROL_H
