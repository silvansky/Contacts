#ifndef PRESENCEPLUGIN_H
#define PRESENCEPLUGIN_H

#include <QSet>
#include <QVariant>
#include <QObjectCleanupHandler>
#include <definitions/optionwidgetorders.h>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/soundfiles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessageprocessor.h>
#include "presence.h"

class PresencePlugin :
			public QObject,
			public IPlugin,
			public IPresencePlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPresencePlugin);
public:
	PresencePlugin();
	~PresencePlugin();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRESENCE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IPresencePlugin
	virtual IPresence *addPresence(IXmppStream *AXmppStream);
	virtual IPresence *getPresence(const Jid &AStreamJid) const;
	virtual bool isContactOnline(const Jid &AContactJid) const;
	virtual QList<Jid> contactsOnline() const;
	virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const;
	virtual void removePresence(IXmppStream *AXmppStream);
signals:
	void streamStateChanged(const Jid &AStreamJid, bool AStateOnline);
	void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void presenceAdded(IPresence *APresence);
	void presenceOpened(IPresence *APresence);
	void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriotity);
	void presenceReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void presenceSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity);
	void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
	void presenceClosed(IPresence *APresence);
	void presenceRemoved(IPresence *APresence);
protected:
	void notifyMoodChanged(IPresence *APresence, const IPresenceItem &AItem);
	void notifyStateChanged(IPresence *APresence, const IPresenceItem &AItem);
protected slots:
	void onPresenceOpened();
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onPresenceClosed();
	void onPresenceDestroyed(QObject *AObject);
	void onStreamAdded(IXmppStream *AXmppStream);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationTest(const QString &ANotificatorId, uchar AKinds);
private:
	IXmppStreams *FXmppStreams;
	IStatusIcons *FStatusIcons;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IMessageProcessor *FMessageProcessor;
private:
	QList<IPresence *> FPresences;
	QObjectCleanupHandler FCleanupHandler;
	QMap<IPresence *, QDateTime> FConnectTime;
	QHash<Jid, QSet<IPresence *> > FContactPresences;
private:
	QMultiMap<IPresence *, int> FNotifies;
	QHash<Jid, QDateTime> FLastMoodNotify;
	QHash<Jid, QDateTime> FLastStateNotify;
};

#endif // PRESENCEPLUGIN_H
