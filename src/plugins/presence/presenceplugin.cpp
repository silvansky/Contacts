#include "presenceplugin.h"

#include <QUrl>
#include <QTextDocument>

#define MOOD_NOTIFY_TIMEOUT           60
#define STATE_NOTIFY_TIMEOUT          60
#define STATE_ROSTERNOTIFY_TIMEOUT    2000
#define CONNECTION_NOTIFY_TIMEOUT     10

PresencePlugin::PresencePlugin()
{
	FXmppStreams = NULL;
	FStatusIcons = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
}

PresencePlugin::~PresencePlugin()
{

}

//IPlugin
void PresencePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Presence Manager");
	APluginInfo->description = tr("Allows other modules to obtain information about the status of contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PresencePlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(added(IXmppStream *)), SLOT(onStreamAdded(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(removed(IXmppStream *)), SLOT(onStreamRemoved(IXmppStream *)));
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
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, uchar)),SLOT(onNotificationTest(const QString &, uchar)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool PresencePlugin::initObjects()
{
	if (FNotifications)
	{
		uchar stateKindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::TestNotify;
		FNotifications->insertNotificator(NID_CONTACT_STATE,OWO_NOTIFICATIONS_STATUS_CHANGES,tr("State Changes"),stateKindMask,0);

		uchar moodKindMask = INotification::PopupWindow|INotification::SoundPlay|INotification::TestNotify;
		FNotifications->insertNotificator(NID_CONTACT_MOOD,OWO_NOTIFICATIONS_MOOD_CHANGES,tr("Mood Changes"),moodKindMask,0);
	}
	return true;
}

//IPresencePlugin
IPresence *PresencePlugin::addPresence(IXmppStream *AXmppStream)
{
	IPresence *presence = getPresence(AXmppStream->streamJid());
	if (!presence)
	{
		presence = new Presence(AXmppStream,FStanzaProcessor);
		connect(presence->instance(),SIGNAL(destroyed(QObject *)),SLOT(onPresenceDestroyed(QObject *)));
		FCleanupHandler.add(presence->instance());
		FPresences.append(presence);
	}
	return presence;
}

IPresence *PresencePlugin::getPresence(const Jid &AStreamJid) const
{
	foreach(IPresence *presence, FPresences)
		if (presence->streamJid() == AStreamJid)
			return presence;
	return NULL;
}

bool PresencePlugin::isContactOnline( const Jid &AContactJid ) const
{
	return FContactPresences.contains(AContactJid);
}

QList<Jid> PresencePlugin::contactsOnline() const
{
	return FContactPresences.keys();
}

QList<IPresence *> PresencePlugin::contactPresences(const Jid &AContactJid) const
{
	return FContactPresences.value(AContactJid).toList();
}

void PresencePlugin::removePresence(IXmppStream *AXmppStream)
{
	IPresence *presence = getPresence(AXmppStream->streamJid());
	if (presence)
	{
		disconnect(presence->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onPresenceDestroyed(QObject *)));
		FPresences.removeAt(FPresences.indexOf(presence));
		delete presence->instance();
	}
}

void PresencePlugin::notifyMoodChanged(IPresence *APresence, const IPresenceItem &AItem)
{
	if (FNotifications && !AItem.itemJid.node().isEmpty() && FConnectTime.contains(APresence))
	{
		QDateTime lastNotify = FLastMoodNotify.value(AItem.itemJid);
		if (lastNotify.isNull() || lastNotify.secsTo(QDateTime::currentDateTime())>=MOOD_NOTIFY_TIMEOUT)
		{
			INotification notify;
			notify.kinds = FNotifications->notificatorKinds(NID_CONTACT_MOOD);
			if (notify.kinds > 0)
			{
				notify.notificatior = NID_CONTACT_MOOD;
				notify.data.insert(NDR_STREAM_JID, APresence->streamJid().full());
				notify.data.insert(NDR_CONTACT_JID, AItem.itemJid.full());
				notify.data.insert(NDR_ICON,FNotifications->contactIcon(APresence->streamJid(),AItem.itemJid));
				notify.data.insert(NDR_POPUP_TITLE, FNotifications->contactName(APresence->streamJid(),AItem.itemJid));
				notify.data.insert(NDR_POPUP_NOTICE,tr("Changed mood"));
				notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AItem.itemJid));
				notify.data.insert(NDR_POPUP_TEXT, Qt::escape(AItem.status));
				notify.data.insert(NDR_SOUND_FILE, SDF_PRESENCE_MOOD_CHANGED);
				FNotifies.insertMulti(APresence,FNotifications->appendNotification(notify));
			}
			FLastMoodNotify.insert(AItem.itemJid,QDateTime::currentDateTime());
		}
	}
}

void PresencePlugin::notifyStateChanged(IPresence *APresence, const IPresenceItem &AItem)
{
	if (FNotifications && !AItem.itemJid.node().isEmpty() && FConnectTime.contains(APresence) && FConnectTime.value(APresence).secsTo(QDateTime::currentDateTime())>CONNECTION_NOTIFY_TIMEOUT)
	{
		INotification notify;
		notify.kinds = FNotifications->notificatorKinds(NID_CONTACT_STATE);

		QDateTime lastNotify = FLastStateNotify.value(AItem.itemJid);
		if (lastNotify.isNull() || lastNotify.secsTo(QDateTime::currentDateTime())>=STATE_NOTIFY_TIMEOUT)
			FLastStateNotify.insert(AItem.itemJid,QDateTime::currentDateTime());
		else
			notify.kinds = notify.kinds & INotification::RosterNotify;

		if (notify.kinds > 0)
		{
			bool isOnline = AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error;
			notify.notificatior = NID_CONTACT_STATE;
			notify.data.insert(NDR_STREAM_JID, APresence->streamJid().full());
			notify.data.insert(NDR_CONTACT_JID, AItem.itemJid.full());
			notify.data.insert(NDR_ROSTER_ORDER,RNO_PRESENCE_CONTACT_STATE);
			notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::AllwaysVisible);
			notify.data.insert(NDR_ROSTER_TIMEOUT, STATE_ROSTERNOTIFY_TIMEOUT);
			notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(isOnline ? Qt::cyan : Qt::lightGray));
			notify.data.insert(NDR_POPUP_ICON, FStatusIcons!=NULL ? FStatusIcons->iconByStatus(isOnline ? IPresence::Online : IPresence::Offline, SUBSCRIPTION_BOTH, false) : QVariant());
			notify.data.insert(NDR_POPUP_TITLE, FNotifications->contactName(APresence->streamJid(),AItem.itemJid));
			notify.data.insert(NDR_POPUP_NOTICE, isOnline ? tr("Connected") : tr("Disconnected"));
			notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AItem.itemJid));
			notify.data.insert(NDR_SOUND_FILE, SDF_PRESENCE_STATE_CHANGED);
			FNotifies.insertMulti(APresence,FNotifications->appendNotification(notify));
		}
	}
}

void PresencePlugin::onPresenceOpened()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		FConnectTime.insert(presence,QDateTime::currentDateTime());
		emit streamStateChanged(presence->streamJid(),true);
		emit presenceOpened(presence);
	}
}

void PresencePlugin::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
		emit presenceChanged(presence,AShow,AStatus,APriority);
}

void PresencePlugin::onPresenceReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		emit presenceReceived(presence,AItem,ABefore);
		if (AItem.show==ABefore.show && AItem.status!=ABefore.status)
		{
			notifyMoodChanged(presence,AItem);
		}
		else if (AItem.show != IPresence::Offline && AItem.show != IPresence::Error)
		{
			QSet<IPresence *> &presences = FContactPresences[AItem.itemJid];
			if (presences.isEmpty())
			{
				notifyStateChanged(presence,AItem);
				emit contactStateChanged(presence->streamJid(),AItem.itemJid,true);
			}
			presences += presence;
		}
		else if (FContactPresences.contains(AItem.itemJid))
		{
			QSet<IPresence *> &presences = FContactPresences[AItem.itemJid];
			presences -= presence;
			if (presences.isEmpty())
			{
				FContactPresences.remove(AItem.itemJid);
				notifyStateChanged(presence,AItem);
				emit contactStateChanged(presence->streamJid(),AItem.itemJid,false);
			}
		}
	}
}

void PresencePlugin::onPresenceSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
		emit presenceSent(presence,AContactJid,AShow,AStatus,APriority);
}

void PresencePlugin::onPresenceAboutToClose(int AShow, const QString &AStatus)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		FConnectTime.remove(presence);
		emit presenceAboutToClose(presence,AShow,AStatus);
	}
}

void PresencePlugin::onPresenceClosed()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		foreach(int notifyId, FNotifies.values(presence))
			FNotifications->removeNotification(notifyId);
		emit streamStateChanged(presence->streamJid(),false);
		emit presenceClosed(presence);
	}
}

void PresencePlugin::onPresenceDestroyed(QObject *AObject)
{
	IPresence *presence = qobject_cast<IPresence *>(AObject);
	FPresences.removeAt(FPresences.indexOf(presence));
}

void PresencePlugin::onStreamAdded(IXmppStream *AXmppStream)
{
	IPresence *presence = addPresence(AXmppStream);
	connect(presence->instance(),SIGNAL(opened()),SLOT(onPresenceOpened()));
	connect(presence->instance(),SIGNAL(changed(int, const QString &, int)),SLOT(onPresenceChanged(int, const QString &, int)));
	connect(presence->instance(),SIGNAL(received(const IPresenceItem &, const IPresenceItem &)),SLOT(onPresenceReceived(const IPresenceItem &, const IPresenceItem &)));
	connect(presence->instance(),SIGNAL(sent(const Jid &, int, const QString &, int)),SLOT(onPresenceSent(const Jid &, int, const QString &, int)));
	connect(presence->instance(),SIGNAL(aboutToClose(int,const QString &)),SLOT(onPresenceAboutToClose(int,const QString &)));
	connect(presence->instance(),SIGNAL(closed()),SLOT(onPresenceClosed()));
	emit presenceAdded(presence);
}

void PresencePlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
	IPresence *presence = getPresence(AXmppStream->streamJid());
	if (presence)
	{
		emit presenceRemoved(presence);
		removePresence(AXmppStream);
	}
}

void PresencePlugin::onNotificationActivated(int ANotifyId)
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

void PresencePlugin::onNotificationRemoved(int ANotifyId)
{
	FNotifies.remove(FNotifies.key(ANotifyId),ANotifyId);
}

void PresencePlugin::onNotificationTest(const QString &ANotificatorId, uchar AKinds)
{
	if (ANotificatorId == NID_CONTACT_MOOD)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.notificatior = ANotificatorId;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/ramblercontacts";
			notify.data.insert(NDR_POPUP_ICON, FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Online, SUBSCRIPTION_BOTH, false) : QVariant());
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_NOTICE,tr("Changed mood"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
			notify.data.insert(NDR_POPUP_TEXT, Qt::escape(tr("Whatever was done, all the better")));
		}
		if (AKinds & INotification::SoundPlay)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_PRESENCE_MOOD_CHANGED);
		}
		if (!notify.data.isEmpty())
		{
			FNotifies.insertMulti(NULL,FNotifications->appendNotification(notify));
		}
	}
	else if (ANotificatorId == NID_CONTACT_STATE)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.notificatior = ANotificatorId;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/ramblercontacts";
			notify.data.insert(NDR_POPUP_ICON, FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Online, SUBSCRIPTION_BOTH, false) :QVariant());
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_NOTICE, tr("Connected"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
		}
		if (AKinds & INotification::SoundPlay)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_PRESENCE_STATE_CHANGED);
		}
		if (!notify.data.isEmpty())
		{
			FNotifies.insertMulti(NULL,FNotifications->appendNotification(notify));
		}
	}
}

Q_EXPORT_PLUGIN2(plg_presence, PresencePlugin)
