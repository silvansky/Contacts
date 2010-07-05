#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QTimer>
#include <QSound>
#include <QPointer>
#include <definations/notificationdataroles.h>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersview.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatusicons.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <utils/action.h>
#include <utils/options.h>
#include "notifywidget.h"
#include "optionswidget.h"
#include "notifykindswidget.h"

struct NotifyRecord {
	NotifyRecord() {
		trayId=0;
		rosterId=0;
		tabPageId=0;
		widget=NULL;
		action=NULL;
	}
	int trayId;
	int rosterId;
	int tabPageId;
	Action *action;
	NotifyWidget *widget;
	INotification notification;
	QPointer<QObject> tabPageNotifier;
};

struct Notificator {
	int order;
	QString title;
	uchar kinds;
	uchar defaults;
	uchar kindMask;
};

class Notifications :
			public QObject,
			public IPlugin,
			public INotifications,
			public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin INotifications IOptionsHolder);
public:
	Notifications();
	~Notifications();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return NOTIFICATIONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//INotifications
	virtual QList<int> notifications() const;
	virtual INotification notificationById(int ANotifyId) const;
	virtual int appendNotification(const INotification &ANotification);
	virtual void activateNotification(int ANotifyId);
	virtual void removeNotification(int ANotifyId);
	//Kind options for notificators
	virtual void insertNotificator(const QString &ANotificatorId, int AWidgetOrder, const QString &ATitle, uchar AKindMask, uchar ADefault);
	virtual uchar notificatorKinds(const QString &ANotificatorId) const;
	virtual void setNotificatorKinds(const QString &ANotificatorId, uchar AKinds);
	virtual void removeNotificator(const QString &ANotificatorId);
	//Notification Utilities
	virtual QImage contactAvatar(const Jid &AContactJid) const;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const;
signals:
	void notificationAppend(int ANotifyId, INotification &ANotification);
	void notificationAppended(int ANotifyId, const INotification &ANotification);
	void notificationActivated(int ANotifyId);
	void notificationRemoved(int ANotifyId);
	void notificationTest(const QString &ANotificatorId, uchar AKinds);
protected:
	int notifyIdByRosterId(int ARosterId) const;
	int notifyIdByTrayId(int ATrayId) const;
	int notifyIdByWidget(NotifyWidget *AWidget) const;
	NotifyWidget* findNotifyWidget(Jid AStreamJid, Jid AContactJid) const;
	void activateAllNotifications();
	void removeAllNotifications();
protected slots:
	void onActivateDelayedActivations();
	void onSoundOnOffActionTriggered(bool);
	void onTrayActionTriggered(bool);
	void onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId);
	void onRosterNotifyRemoved(IRosterIndex *AIndex, int ANotifyId);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onTrayNotifyRemoved(int ANotifyId);
	void onWindowNotifyActivated();
	void onWindowNotifyRemoved();
	void onWindowNotifyDestroyed();
	void onActionNotifyActivated(bool);
	void onTestNotificationTimerTimedOut();
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void showSettings();
private:
	IAvatars *FAvatars;
	IRosterPlugin *FRosterPlugin;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	ITrayManager *FTrayManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IOptionsManager *FOptionsManager;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	Action *FSoundOnOff;
	Action *FActivateAll;
	Action *FRemoveAll;
	Menu *FNotifyMenu;
private:
	int FNotifyId;
	int FTestNotifyId;
	QSound *FSound;
	QTimer FTestNotifyTimer;
	QList<int> FDelayedActivations;
	QMap<int, NotifyRecord> FNotifyRecords;
	mutable QMap<QString, Notificator> FNotificators;
};

#endif // NOTIFICATIONS_H
