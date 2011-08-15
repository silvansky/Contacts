#ifndef ROSTERPLUGIN_H
#define ROSTERPLUGIN_H

#include <QObjectCleanupHandler>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/soundfiles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessageprocessor.h>
#include "roster.h"

class RosterPlugin :
	public QObject,
	public IPlugin,
	public IRosterPlugin
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IRosterPlugin)
public:
	RosterPlugin();
	~RosterPlugin();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRosterPlugin
	virtual IRoster *addRoster(IXmppStream *AXmppStream);
	virtual IRoster *getRoster(const Jid &AStreamJid) const;
	virtual QString rosterFileName(const Jid &AStreamJid) const;
	virtual void removeRoster(IXmppStream *AXmppStream);
signals:
	void rosterAdded(IRoster *ARoster);
	void rosterOpened(IRoster *ARoster);
	void rosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void rosterSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void rosterSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void rosterClosed(IRoster *ARoster);
	void rosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
	void rosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void rosterRemoved(IRoster *ARoster);
protected:
	void notifyContactAdded(IRoster *ARoster, const IRosterItem &AItem);
protected slots:
	void onRosterOpened();
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterSubscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText);
	void onRosterSubscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText);
	void onRosterClosed();
	void onRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void onRosterStreamJidChanged(const Jid &ABefour);
	void onRosterDestroyed(QObject *AObject);
	void onStreamAdded(IXmppStream *AStream);
	void onStreamRemoved(IXmppStream *AStream);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IMessageProcessor *FMessageProcessor;
private:
	QList<IRoster *> FRosters;
	QObjectCleanupHandler FCleanupHandler;
	QMultiMap<IRoster *, int> FNotifies;
};

#endif // ROSTERPLUGIN_H
