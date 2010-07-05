#ifndef NOTIFYKINDSWIDGET_H
#define NOTIFYKINDSWIDGET_H

#include <QWidget>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_notifykindswidget.h"

class NotifyKindsWidget :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	NotifyKindsWidget(INotifications *ANotifications, const QString &ANotificatorId, const QString &ATitle, uchar AKindMask, QWidget *AParent);
	~NotifyKindsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
signals:
	void notificationTest(const QString &ANotificatorId, uchar AKinds);
protected:
	uchar changedKinds(uchar AActiveKinds) const;
protected slots:
	void onTestLinkActivated(const QString &ALink);
private:
	Ui::NotifyKindsWidgetClass ui;
private:
	INotifications *FNotifications;
private:
	QString FNotificatorId;
	uchar FNotificatorKindMask;
};

#endif // NOTIFYKINDSWIDGET_H
