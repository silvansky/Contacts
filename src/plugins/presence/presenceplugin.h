#ifndef PRESENCEPLUGIN_H
#define PRESENCEPLUGIN_H

#include <QSet>
#include <QVariant>
#include <QObjectCleanupHandler>
#include <definitions/optionwidgetorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/soundfiles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imetacontacts.h>
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
	virtual IPresence *getPresence(IXmppStream *AXmppStream);
	virtual IPresence *findPresence(const Jid &AStreamJid) const;
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
	void presenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void presenceDirectSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity);
	void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
	void presenceClosed(IPresence *APresence);
	void presenceRemoved(IPresence *APresence);
protected:
	bool isNotifyAvailable(IPresence *APresence, const Jid &AContactJid) const;
	QDateTime lastNotifyTime(IPresence *APresence, const Jid &AContactJid, const QHash<Jid, QDateTime> &ALastNotify) const;
	void notifyMoodChanged(IPresence *APresence, const IPresenceItem &AItem);
	void notifyStateChanged(IPresence *APresence, const IPresenceItem &AItem);
protected slots:
	void onPresenceOpened();
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceDirectSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onPresenceClosed();
	void onPresenceDestroyed(QObject *AObject);
	void onStreamAdded(IXmppStream *AXmppStream);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationTest(const QString &ATypeId, ushort AKinds);
private:
	IGateways *FGateways;
	IXmppStreams *FXmppStreams;
	IStatusIcons *FStatusIcons;
	IMetaContacts *FMetaContacts;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IMessageProcessor *FMessageProcessor;
private:
	QList<IPresence *> FPresences;
	QObjectCleanupHandler FCleanupHandler;
	QHash<Jid, QSet<IPresence *> > FContactPresences;
private:
	QMultiMap<IPresence *, int> FNotifies;
	QHash<Jid, QDateTime> FLastMoodNotify;
	QHash<Jid, QDateTime> FLastStateNotify;
	QMap<IPresence *, QDateTime> FConnectTime;
};

#endif // PRESENCEPLUGIN_H
