#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#ifdef USE_PHONON
#	include <Phonon/Phonon>
#else
#	include <QSound>
#endif

#include <QTimer>
#include <definitions/notificationdataroles.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersview.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/igateways.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istatusicons.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/isystemintegration.h>
#include <utils/options.h>
#include <utils/systemmanager.h>
#include "notifywidget.h"
#include "notifykindswidget.h"

struct NotifyRecord
{
	NotifyRecord() {
		trayId = 0;
		rosterId = 0;
		tabPageId = 0;
	}
	int trayId;
	int rosterId;
	int tabPageId;
	INotification notification;
	QPointer<QObject> tabPageNotifier;
	QPointer<NotifyWidget> popupWidget;
};

class Notifications :
	public QObject,
	public IPlugin,
	public INotifications,
	public IOptionsHolder
{
	Q_OBJECT
	Q_INTERFACES(IPlugin INotifications IOptionsHolder)
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
	virtual void registerNotificationType(const QString &ATypeId, const INotificationType &AType);
	virtual QList<QString> notificationTypes() const;
	virtual INotificationType notificationType(const QString &ATypeId) const;
	virtual void removeNotificationType(const QString &ATypeId);
	virtual ushort enabledNotificationKinds() const;
	virtual void setEnabledNotificationKinds(ushort AKinds);
	virtual ushort enabledTypeNotificationKinds(const QString &ATypeId) const;
	virtual ushort typeNotificationKinds(const QString &ATypeId) const;
	virtual void setTypeNotificationKinds(const QString &ATypeId, ushort AKinds);
	//Notification Utilities
	virtual QImage contactAvatar(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const;
signals:
	void notificationActivated(int ANotifyId);
	void notificationRemoved(int ANotifyId);
	void notificationAppend(int ANotifyId, INotification &ANotification);
	void notificationAppended(int ANotifyId, const INotification &ANotification);
	void notificationTest(const QString &ATypeId, ushort AKinds);
protected:
	int notifyIdByRosterId(int ARosterId) const;
	int notifyIdByTrayId(int ATrayId) const;
	int notifyIdByWidget(NotifyWidget *AWidget) const;
	void activateAllNotifications(uint kinds = INotification::TabPageNotify);
	void activateFirstNotification(uint kinds = INotification::TabPageNotify);
	void removeAllNotifications();
	void removeInvisibleNotification(int ANotifyId);
protected slots:
	void onActivateDelayedActivations();
	void onActivateDelayedReplaces();
	void onTrayActionTriggered(bool);
	void onRosterNotifyActivated(int ANotifyId);
	void onRosterNotifyTimeout(int ANotifyId);
	void onRosterNotifyRemoved(int ANotifyId);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onTrayNotifyRemoved(int ANotifyId);
	void onWindowNotifyActivated();
	void onWindowNotifyRemoved();
	void onWindowNotifyOptions();
	void onWindowNotifyDestroyed();
	void onTestNotificationTimerTimedOut();
	void onSystemNotifyClicked(int ANotifyId);
	void onShowSystemNotificationsSettings();
	void onNotifyCountChanged();
	void onDockClicked();
private:
	IAvatars *FAvatars;
	IGateways *FGateways;
	IRosterPlugin *FRosterPlugin;
	IMetaContacts *FMetaContacts;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	ITrayManager *FTrayManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IOptionsManager *FOptionsManager;
	ISystemIntegration * FSystemIntegration;
	IMainWindowPlugin * FMainWindow;
private:
#ifdef USE_PHONON
	Phonon::MediaObject *FMediaObject;
	Phonon::AudioOutput *FAudioOutput;
#else
	QSound *FSound;
#endif
private:
	int FNotifyId;
	int FTestNotifyId;
	Action *FActivateAll;
	QTimer FTestNotifyTimer;
	QList<int> FDelayedReplaces;
	QList<int> FDelayedActivations;
	QMap<int, NotifyRecord> FNotifyRecords;
	QMap<QString, INotificationType> FNotifyTypes;
};

#endif // NOTIFICATIONS_H
