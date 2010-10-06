#ifndef ROSTERTOOLTIP_H
#define ROSTERTOOLTIP_H

#include <QFrame>
#include <QBasicTimer>
#include <definations/resources.h>
#include <definations/stylesheets.h>
#include <utils/stylestorage.h>
#include <utils/toolbarchanger.h>
#include "ui_rostertooltip.h"

class RosterToolTip : 
	public QFrame
{
	Q_OBJECT;
public:
	RosterToolTip(QWidget *AParent);
	~RosterToolTip();
	static void createInstance(const QPoint &APos, QWidget *AWidget);
	static ToolBarChanger *toolBarChanger();
	static void showTip(const QPoint &APos, const QString &AText, QWidget *AWidget, const QRect &ARect = QRect());
protected:
	void hideTip();
	void hideTipImmediately();
	void restartExpireTimer();
	void reuseTip(const QString &AText);
	void placeTip(const QPoint &APos, QWidget *AWidget);
	void setTipRect(QWidget *AWidget, const QRect &ARect);
	bool isTipChanged(const QPoint &APos, const QString &AText, QObject *AObject);
	static int getTipScreen(const QPoint &APos, QWidget *AWidget);
protected:
	void paintEvent(QPaintEvent *AEvent);
	void resizeEvent(QResizeEvent *AEvent);
	void timerEvent(QTimerEvent *AEvent);
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	bool eventFilter(QObject *AWatch, QEvent *AEvent);
private:
	Ui::RosterToolTipClass ui;
private:
	static RosterToolTip *instance;
private:
	bool FMouseOver;
	QRect FRect;
	QWidget *FWidget;
	QBasicTimer FHideTimer;
	QBasicTimer FExpireTimer;
	ToolBarChanger *FToolBarChanger;
};

#endif // ROSTERTOOLTIP_H
