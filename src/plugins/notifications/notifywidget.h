#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QMouseEvent>
#include <QDesktopWidget>
#include <definations/notificationdataroles.h>
#include <interfaces/inotifications.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
#include "ui_notifywidget.h"

class NotifyWidget :
			public QWidget
{
	Q_OBJECT;
public:
	NotifyWidget(const INotification &ANotification);
	~NotifyWidget();
	void appear();
	void animateTo(int AYPos);
	INotification notification() const;
	void setNotification(const INotification & notification);
	void appendText(const QString & text);
	static NotifyWidget* findNotifyWidget(Jid AStreamJid, Jid AContactJid);
signals:
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
	void closeButtonCLicked();
	void settingsButtonCLicked();
protected:
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *);
protected slots:
	void onAnimateStep();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
	INotification FNotification;
	QStringList FTextMessages;
	QTimer * deleteTimer;
private:
	static void layoutWidgets();
	static QDesktopWidget *FDesktop;
	static QList<NotifyWidget *> FWidgets;
};

#endif // NOTIFYWIDGET_H
