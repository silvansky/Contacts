#include "presenceplugin.h"

#include <QTextDocument>

#define MOOD_NOTIFICATOR_ID      "MoodChanged"
#define STATE_NOTIFICATOR_ID     "StateChanged"

PresencePlugin::PresencePlugin()
{
	FXmppStreams = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
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
	APluginInfo->homePage = "http://virtus.rambler.ru";
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
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivatedOrRemoved(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationActivatedOrRemoved(int)));
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, uchar)),SLOT(onNotificationTest(const QString &, uchar)));
		}
	}
	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool PresencePlugin::initObjects()
{
	if (FNotifications)
	{
		uchar kindMask = INotification::PopupWindow|INotification::PlaySound|INotification::TestNotify;
		uchar kindDefs = 0;
		FNotifications->insertNotificator(STATE_NOTIFICATOR_ID,OWO_NOTIFICATIONS_STATUS_CHANGES,tr("State Changes"),kindMask,kindDefs);
		FNotifications->insertNotificator(MOOD_NOTIFICATOR_ID,OWO_NOTIFICATIONS_MOOD_CHANGES,tr("Mood Changes"),kindMask,kindDefs);
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
	if (FNotifications && APresence->isOpen() && !AItem.itemJid.node().isEmpty())
	{
		INotification notify;
		notify.kinds = FNotifications->notificatorKinds(MOOD_NOTIFICATOR_ID);
		notify.data.insert(NDR_STREAM_JID, APresence->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID, AItem.itemJid.full());
		notify.data.insert(NDR_POPUP_CAPTION,tr("Changed mood to"));
		notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AItem.itemJid));
		notify.data.insert(NDR_POPUP_TITLE, FNotifications->contactName(APresence->streamJid(),AItem.itemJid));
		notify.data.insert(NDR_POPUP_TEXT, tr("Mood:")+"<br>"+Qt::escape(AItem.status));
		notify.data.insert(NDR_SOUND_FILE, SDF_PRESENCE_MOOD_CHANGED);
		FNotifies.append(FNotifications->appendNotification(notify));
	}
}

void PresencePlugin::notifyStateChanged(IPresence *APresence, const IPresenceItem &AItem)
{
	if (FNotifications && APresence->isOpen() && !AItem.itemJid.node().isEmpty())
	{
		bool isOnline = AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error;

		INotification notify;
		notify.kinds = FNotifications->notificatorKinds(STATE_NOTIFICATOR_ID);
		notify.data.insert(NDR_STREAM_JID, APresence->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID, AItem.itemJid.full());
		notify.data.insert(NDR_POPUP_CAPTION, isOnline ? tr("Connected") : tr("Disconnected"));
		notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AItem.itemJid));
		notify.data.insert(NDR_POPUP_TITLE, FNotifications->contactName(APresence->streamJid(),AItem.itemJid));
		notify.data.insert(NDR_SOUND_FILE, SDF_PRESENCE_STATE_CHANGED);
		FNotifies.append(FNotifications->appendNotification(notify));
	}
}

void PresencePlugin::onPresenceOpened()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
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
		emit presenceReceived(presence,AItem,ABefore);
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
		emit presenceAboutToClose(presence,AShow,AStatus);
}

void PresencePlugin::onPresenceClosed()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
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

void PresencePlugin::onNotificationActivatedOrRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifications->removeNotification(ANotifyId);
		FNotifies.removeAll(ANotifyId);
	}
}

void PresencePlugin::onNotificationTest(const QString &ANotificatorId, uchar AKinds)
{
	if (ANotificatorId == MOOD_NOTIFICATOR_ID)
	{
		INotification notify;
		notify.kinds = AKinds;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/virtus";
			notify.data.insert(NDR_STREAM_JID,contactJid.full());
			notify.data.insert(NDR_CONTACT_JID,contactJid.full());
			notify.data.insert(NDR_POPUP_CAPTION,tr("Changed mood to"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_TEXT,tr("Mood:")+"<br>"+Qt::escape(tr("Whatever was done, all the better")));
		}
		if (AKinds & INotification::PlaySound)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_PRESENCE_MOOD_CHANGED);
		}
		if (!notify.data.isEmpty())
		{
			FNotifies.append(FNotifications->appendNotification(notify));
		}
	}
	else if (ANotificatorId == STATE_NOTIFICATOR_ID)
	{
		INotification notify;
		notify.kinds = AKinds;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/virtus";
			notify.data.insert(NDR_STREAM_JID,contactJid.full());
			notify.data.insert(NDR_CONTACT_JID,contactJid.full());
			notify.data.insert(NDR_POPUP_CAPTION,tr("Connected"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
		}
		if (AKinds & INotification::PlaySound)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_PRESENCE_STATE_CHANGED);
		}
		if (!notify.data.isEmpty())
		{
			FNotifies.append(FNotifications->appendNotification(notify));
		}
	}
}

Q_EXPORT_PLUGIN2(plg_presence, PresencePlugin)
