#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QMouseEvent>
#include <QDesktopWidget>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/notificationdataroles.h>
#include <interfaces/inotifications.h>
#include <utils/message.h>
#include <utils/actionbutton.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_notifywidget.h"

class CustomBorderContainer;

class NotifyWidget :
			public QWidget
{
	Q_OBJECT
public:
	NotifyWidget(const INotification &ANotification, bool AOptionsAvailable);
	~NotifyWidget();
	void appear();
	void animateTo(int AYPos);
	void appendAction(Action *AAction);
	void appendNotification(const INotification &ANotification);
public slots:
	void adjustHeight();
	void updateElidedText();
signals:
	void showOptions();
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
protected:
	virtual void enterEvent(QEvent *AEvent);
	virtual void leaveEvent(QEvent *AEvent);
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *AEvent);
protected slots:
	void onAnimateStep();
	void onCloseTimerTimeout();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
	QTimer *FCloseTimer;
	CustomBorderContainer * border;
private:
	QString FTitle;
	QString FNotice;
	QString FCaption;
	QStringList FTextMessages;
private:
	static void layoutWidgets();
	static QDesktopWidget *FDesktop;
	static QList<NotifyWidget *> FWidgets;
};

#endif // NOTIFYWIDGET_H
