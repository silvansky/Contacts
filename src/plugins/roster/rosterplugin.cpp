#include "rosterplugin.h"

#include <QDir>

#define DIR_ROSTERS   "rosters"

RosterPlugin::RosterPlugin()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
}

RosterPlugin::~RosterPlugin()
{

}

void RosterPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Manager");
	APluginInfo->description = tr("Allows other modules to get information about contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool RosterPlugin::initObjects()
{
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_CONTACT_ADDED;
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_CONTACT_ADDED,notifyType);
	}
	return true;
}

//IRosterPlugin
IRoster *RosterPlugin::addRoster(IXmppStream *AXmppStream)
{
	IRoster *roster = getRoster(AXmppStream->streamJid());
	if (!roster)
	{
		roster = new Roster(AXmppStream, FStanzaProcessor);
		connect(roster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
		FCleanupHandler.add(roster->instance());
		FRosters.append(roster);
	}
	return roster;
}

IRoster *RosterPlugin::getRoster(const Jid &AStreamJid) const
{
	foreach(IRoster *roster, FRosters)
		if (roster->streamJid() == AStreamJid)
			return roster;
	return NULL;
}

QString RosterPlugin::rosterFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_ROSTERS))
		dir.mkdir(DIR_ROSTERS);
	dir.cd(DIR_ROSTERS);

	return dir.absoluteFilePath(Jid::encode(AStreamJid.bare()).toLower()+".xml");
}

void RosterPlugin::removeRoster(IXmppStream *AXmppStream)
{
	IRoster *roster = getRoster(AXmppStream->streamJid());
	if (roster)
	{
		disconnect(roster->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onRosterDestroyed(QObject *)));
		FRosters.removeAt(FRosters.indexOf(roster));
		delete roster->instance();
	}
}

void RosterPlugin::notifyContactAdded(IRoster *ARoster, const IRosterItem &AItem)
{
	if (FNotifications && ARoster->isOpen() && !AItem.itemJid.node().isEmpty())
	{
		INotification notify;
		notify.kinds = FNotifications->notificationKinds(NNT_CONTACT_ADDED);
		if (notify.kinds > 0)
		{
			notify.typeId = NNT_CONTACT_ADDED;
			notify.data.insert(NDR_STREAM_JID, ARoster->streamJid().full());
			notify.data.insert(NDR_CONTACT_JID, AItem.itemJid.full());
			notify.data.insert(NDR_POPUP_TITLE, FNotifications->contactName(ARoster->streamJid(),AItem.itemJid));
			notify.data.insert(NDR_POPUP_NOTICE,tr("Added to the contacts list"));
			notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(ARoster->streamJid(),AItem.itemJid));
			notify.data.insert(NDR_SOUND_FILE, SDF_ROSTER_CONTACT_ADDED);
			FNotifies.insertMulti(ARoster,FNotifications->appendNotification(notify));
		}
	}
}

void RosterPlugin::onRosterOpened()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterOpened(roster);
}

void RosterPlugin::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		//if (AItem.subscription!=SUBSCRIPTION_REMOVE && !ABefore.isValid)
		//	notifyContactAdded(roster,AItem);
		emit rosterItemReceived(roster,AItem,ABefore);
	}
}

void RosterPlugin::onRosterSubscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionSent(roster,AItemJid,ASubsType,AText);
}

void RosterPlugin::onRosterSubscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionReceived(roster,AItemJid,ASubsType,AText);
}

void RosterPlugin::onRosterClosed()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		foreach(int notifyId, FNotifies.values(roster))
			FNotifications->removeNotification(notifyId);
		emit rosterClosed(roster);
	}
}

void RosterPlugin::onRosterStreamJidAboutToBeChanged(const Jid &AAfter)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		if (!(roster->streamJid() && AAfter))
			roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterStreamJidAboutToBeChanged(roster,AAfter);
	}
}

void RosterPlugin::onRosterStreamJidChanged(const Jid &ABefour)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		emit rosterStreamJidChanged(roster,ABefour);
		if (!(roster->streamJid() && ABefour))
			roster->loadRosterItems(rosterFileName(roster->streamJid()));
	}
}

void RosterPlugin::onRosterDestroyed(QObject *AObject)
{
	IRoster *roster = qobject_cast<IRoster *>(AObject);
	FRosters.removeAt(FRosters.indexOf(roster));
}

void RosterPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
	IRoster *roster = addRoster(AXmppStream);
	connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
	connect(roster->instance(),SIGNAL(received(const IRosterItem &,const IRosterItem &)),SLOT(onRosterItemReceived(const IRosterItem &,const IRosterItem &)));
	connect(roster->instance(),SIGNAL(subscriptionSent(const Jid &, int, const QString &)),
		SLOT(onRosterSubscriptionSent(const Jid &, int, const QString &)));
	connect(roster->instance(),SIGNAL(subscriptionReceived(const Jid &, int, const QString &)),
		SLOT(onRosterSubscriptionReceived(const Jid &, int, const QString &)));
	connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
	connect(roster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),SLOT(onRosterStreamJidAboutToBeChanged(const Jid &)));
	connect(roster->instance(),SIGNAL(streamJidChanged(const Jid &)),SLOT(onRosterStreamJidChanged(const Jid &)));
	emit rosterAdded(roster);
	roster->loadRosterItems(rosterFileName(roster->streamJid()));
}

void RosterPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
	IRoster *roster = getRoster(AXmppStream->streamJid());
	if (roster)
	{
		roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterRemoved(roster);
		removeRoster(AXmppStream);
	}
}

void RosterPlugin::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.values().contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			INotification notify = FNotifications->notificationById(ANotifyId);
			FMessageProcessor->createMessageWindow(notify.data.value(NDR_STREAM_JID).toString(),notify.data.value(NDR_CONTACT_JID).toString(),Message::Chat,IMessageHandler::SM_SHOW);
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void RosterPlugin::onNotificationRemoved(int ANotifyId)
{
	FNotifies.remove(FNotifies.key(ANotifyId),ANotifyId);
}

Q_EXPORT_PLUGIN2(plg_roster, RosterPlugin)
