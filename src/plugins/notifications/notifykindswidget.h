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
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)
public:
	NotifyKindsWidget(INotifications *ANotifications, const QString &ATypeId, const QString &ATitle, ushort AKindMask, ushort AKindDefs, QWidget *AParent);
	~NotifyKindsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void updated();
	void childApply();
	void childReset();
signals:
	void notificationTest(const QString &ANotificatorId, ushort AKinds);
protected:
	ushort changedKinds(ushort AActiveKinds) const;
protected slots:
	void onTestLinkActivated(const QString &ALink);
	void onTestButtonClicked();
	void onModified();
private:
	Ui::NotifyKindsWidgetClass ui;
private:
	INotifications *FNotifications;
private:
	QString FTypeId;
	ushort FKindMask;
	ushort FKindDefs;
};

#endif // NOTIFYKINDSWIDGET_H
