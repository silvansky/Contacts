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
	NotifyWidget(const INotification &ANotification);
	~NotifyWidget();
	bool appear();
	void animateTo(int AYPos);
	void appendAction(Action *AAction);
	void appendNotification(const INotification &ANotification);
signals:
	void showOptions();
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
protected:
	void updateElidedText();
protected:
	virtual void enterEvent(QEvent *AEvent);
	virtual void leaveEvent(QEvent *AEvent);
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onAdjustHeight();
	void onAnimateStep();
	void onCloseTimerTimeout();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
	bool FCanActivate;
	bool FFirstLayout;
	QTimer *FCloseTimer;
	CustomBorderContainer *FBorder;
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
