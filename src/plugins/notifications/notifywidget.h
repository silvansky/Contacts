#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QFrame>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/notificationdataroles.h>
#include <interfaces/inotifications.h>
#include <utils/actionbutton.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_notifywidget.h"

class NotifyWidget :
			public QFrame
{
	Q_OBJECT;
public:
	NotifyWidget(const INotification &ANotification);
	~NotifyWidget();
	void appear();
	void animateTo(int AYPos);
	void appendAction(Action *AAction);
	void appendNotification(const INotification &ANotification);
public slots:
	void adjustHeight();
signals:
	void showOptions();
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
protected:
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *AEvent);
protected slots:
	void onAnimateStep();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
	QTimer *FCloseTimer;
	QStringList FTextMessages;
private:
	static void layoutWidgets();
	static QDesktopWidget *FDesktop;
	static QList<NotifyWidget *> FWidgets;
};

#endif // NOTIFYWIDGET_H
