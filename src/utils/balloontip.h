#ifndef BALLOONTIP_H
#define BALLOONTIP_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT BalloonTip :
			public QWidget
{
	Q_OBJECT
public:
	enum ArrowPosition
	{
		AutoArrow,
		ArrowLeft,
		ArrowRight,
		ArrowTop,
		ArrowBottom
	};
	static bool isBalloonVisible();
	static QWidget *showBalloon(QIcon icon, const QString& title, const QString& msg,
		const QPoint& pos, int timeout = 10000, bool showArrow = true, ArrowPosition arrowPosition = AutoArrow, QWidget * p = NULL);
	// warning! balloon does not take ownership of messageWidget
	static QWidget *showBalloon(QIcon icon, QWidget * messageWidget,
		const QPoint& pos, int timeout = 10000, bool showArrow = true, ArrowPosition arrowPosition = AutoArrow, QWidget * p = NULL);
	static void hideBalloon();
signals:
	void messageClicked();
	void closed();
private:
	void init();
	BalloonTip(QIcon icon, const QString& title, const QString& msg, QWidget * p);
	BalloonTip(QIcon icon, QWidget * messageWidget, QWidget * p);
	~BalloonTip();
	void drawBalloon(const QPoint& pos, int timeout = 10000, bool showArrow = true, ArrowPosition arrowPosition = AutoArrow);
protected:
	void paintEvent(QPaintEvent *ev);
	void mousePressEvent(QMouseEvent *ev);
	void timerEvent(QTimerEvent *ev);
	bool event(QEvent * ev);
	bool eventFilter(QObject *, QEvent *);
private:
	int timerId;
	QPixmap pixmap;
	QWidget * widget;
	QWidget * _p; // quasi parent, watch it for deactivation
	static BalloonTip * theSolitaryBalloonTip;
};

#endif // BALLOONTIP_H
