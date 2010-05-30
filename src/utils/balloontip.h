#ifndef BALLOONTIP_H
#define BALLOONTIP_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT BalloonTip : 
			public QWidget
{
	Q_OBJECT;
public:
	static bool isBalloonVisible();
	static QWidget *showBalloon(QIcon icon, const QString& title, const QString& msg, 
		const QPoint& pos, int timeout = 10000, bool showArrow = true);
	static void hideBalloon();
signals:
	void messageClicked();
	void closed();
private:
	BalloonTip(QIcon icon, const QString& title, const QString& msg);
	~BalloonTip();
	void drawBalloon(const QPoint& pos, int timeout = 10000, bool showArrow = true);
protected:
	void paintEvent(QPaintEvent *ev);
	void mousePressEvent(QMouseEvent *ev);
	void timerEvent(QTimerEvent *ev);
private:
	int timerId;
	QPixmap pixmap;
};

#endif // BALLOONTIP_H
