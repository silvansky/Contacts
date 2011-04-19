#include "ramblermailnotify.h"

#define MAIL_INDEX_ID "rambler.mail.notify"
#define SHC_MAIL_NOTIFY     "/message/x[@xmlns='"NS_RAMBLER_MAIL_NOTIFY"']"

RamblerMailNotify::RamblerMailNotify()
{
	FGateways = NULL;
	FRosterPlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FStatusIcons = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
	
	FSHIMailNotify = -1;
}

RamblerMailNotify::~RamblerMailNotify()
{

}

void RamblerMailNotify::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Rambler Mail Notifier");
	APluginInfo->description = tr("Notify of new e-mails in Rambler mail box");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RamblerMailNotify::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IGateways").value(0);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterStateChanged(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterStateChanged(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		FRostersView = rostersViewPlugin!=NULL ? rostersViewPlugin->rostersView() : NULL;
		if (FRostersView)
		{
			connect(FRostersView->instance(),SIGNAL(notifyActivated(int)),SLOT(onRosterNotifyActivated(int)));
			connect(FRostersView->instance(),SIGNAL(notifyRemoved(int)),SLOT(onRosterNotifyRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(streamAdded(const Jid &)),SLOT(onStreamAdded(const Jid &)));
			connect(FRostersModel->instance(),SIGNAL(streamRemoved(const Jid &)),SLOT(onStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	return FStanzaProcessor != NULL;
}

bool RamblerMailNotify::initObjects()
{
	if (FRostersView)
	{
		IRostersLabel rlabel;
		rlabel.order = RLO_AVATAR_IMAGE;
		rlabel.label = RDR_AVATAR_IMAGE;
		FAvatarLabelId = FRostersView->registerLabel(rlabel);
	}
	if (FNotifications)
	{
		uchar kindMask = INotification::PopupWindow|INotification::PlaySoundNotification;
		FNotifications->insertNotificator(NID_MAIL_NOTIFY,OWO_NOTIFICATIONS_MAIL_NOTIFY,tr("Mail Notifies"),kindMask,kindMask);
	}
	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_MI_MAIL_NOTIFY;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.conditions.append(SHC_MAIL_NOTIFY);
		FSHIMailNotify = FStanzaProcessor->insertStanzaHandle(handle);
	}
	return true;
}

bool RamblerMailNotify::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandleId == FSHIMailNotify)
	{
		IDiscoIdentity identity;
		identity.category = "gateway";
		identity.type = "mail";
		if (FGateways && FGateways->streamServices(AStreamJid,identity).contains(AStanza.from()))
		{
			AAccept = true;
			insertMailNotify(AStreamJid,AStanza);
		}
		return true;
	}
	return false;
}

IRosterIndex *RamblerMailNotify::findMailIndex(const Jid &AStreamJid) const
{
	foreach(IRosterIndex *index, FMailIndexes)
		if (index->data(RDR_STREAM_JID).toString() == AStreamJid.pFull())
			return index;
	return NULL;
}

void RamblerMailNotify::updateMailIndex(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (mindex)
	{
		IRosterIndex *sindex = FRostersModel->streamRoot(AStreamJid);
		int show = sindex!=NULL ? sindex->data(RDR_SHOW).toInt() : IPresence::Offline;
		QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByStatus(show!=IPresence::Offline && show!=IPresence::Error ? IPresence::Online : IPresence::Offline,SUBSCRIPTION_BOTH,false) : QIcon();
		mindex->setData(Qt::DecorationRole, icon);
	}
}

void RamblerMailNotify::insertMailNotify(const Jid &AStreamJid, const Stanza &AStanza)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (FNotifications && mindex)
	{
		QDomElement xElem = AStanza.firstElement("x",NS_RAMBLER_MAIL_NOTIFY);

		if (FRostersView)
		{
			IRostersNotify rnotify;
			rnotify.order = RNO_RAMBLER_MAIL_NOTIFY;
			rnotify.flags = IRostersNotify::Blink|IRostersNotify::AllwaysVisible;
			rnotify.hookClick = false;
			rnotify.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RAMBLERMAILNOTIFY_NOTIFY);
			rnotify.footer = tr("%n new message").arg(FIndexPopupNotifies.values(mindex).count()+1);
			rnotify.background = QBrush(Qt::yellow);
			FRostersView->removeNotify(FIndexRosterNotify.take(mindex));
			FIndexRosterNotify.insert(mindex,FRostersView->insertNotify(rnotify,QList<IRosterIndex *>()<<mindex));
		}

		INotification notify;
		notify.kinds = FNotifications->notificatorKinds(NID_MAIL_NOTIFY);
		if ((notify.kinds & (INotification::PopupWindow|INotification::PlaySoundNotification))>0)
		{
			notify.notificatior = NID_MAIL_NOTIFY;
			notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
			notify.data.insert(NDR_CONTACT_JID,xElem.firstChildElement("contact").text());
			notify.data.insert(NDR_POPUP_CAPTION,tr("New E-mail from"));
			notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
			notify.data.insert(NDR_POPUP_TITLE,xElem.firstChildElement("from").text());
			notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
			notify.data.insert(NDR_POPUP_TEXT,AStanza.firstElement("subject").text());

			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
			if (roster && !roster->rosterItem(xElem.firstChildElement("contact").text()).isValid)
				notify.data.insert(NDR_POPUP_NOTICE,tr("Not in contact list"));

			FIndexPopupNotifies.insertMulti(mindex,FNotifications->appendNotification(notify));
		}
	}
}

void RamblerMailNotify::onStreamAdded(const Jid &AStreamJid)
{
	IRosterIndex *sroot = FRostersModel->streamRoot(AStreamJid);
	if (sroot)
	{
		IRosterIndex *mindex = FRostersModel->createRosterIndex(RIT_MAILNOTIFY,MAIL_INDEX_ID,sroot);
		mindex->setData(Qt::DisplayRole,tr("Mail"));
		mindex->setData(RDR_TYPE_ORDER,RITO_MAILNOTIFY);
		mindex->setData(RDR_AVATAR_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
		if (FRostersView)
		{
			FRostersView->insertLabel(FAvatarLabelId,mindex);
			FRostersView->insertFooterText(FTO_ROSTERSVIEW_STATUS,tr("No new messages"),mindex);
		}
		FMailIndexes.append(mindex);
		FRostersModel->insertRosterIndex(mindex,sroot);
		updateMailIndex(AStreamJid);
	}
}

void RamblerMailNotify::onStreamRemoved(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	FMailIndexes.removeAll(mindex);
}

void RamblerMailNotify::onRosterStateChanged(IRoster *ARoster)
{
	updateMailIndex(ARoster->streamJid());
}

void RamblerMailNotify::onNotificationActivated(int ANotifyId)
{
	if (FIndexPopupNotifies.values().contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			Jid streamJid = FNotifications->notificationById(ANotifyId).data.value(NDR_STREAM_JID).toString();
			Jid contactJid = FNotifications->notificationById(ANotifyId).data.value(NDR_CONTACT_JID).toString();
			FMessageProcessor->createWindow(streamJid, contactJid, Message::Chat, IMessageHandler::SM_SHOW);
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void RamblerMailNotify::onNotificationRemoved(int ANotifyId)
{
	if (FIndexPopupNotifies.values().contains(ANotifyId))
	{
		IRosterIndex *mindex = FIndexPopupNotifies.key(ANotifyId);
		FIndexPopupNotifies.remove(mindex,ANotifyId);
	}
}

void RamblerMailNotify::onRosterNotifyActivated(int ANotifyId)
{
	if (FIndexRosterNotify.values().contains(ANotifyId))
	{
		FRostersView->removeNotify(ANotifyId);
	}
}

void RamblerMailNotify::onRosterNotifyRemoved(int ANotifyId)
{
	if (FIndexRosterNotify.values().contains(ANotifyId))
	{
		IRosterIndex *mindex = FIndexRosterNotify.key(ANotifyId);
		FIndexRosterNotify.remove(mindex);
	}
}

Q_EXPORT_PLUGIN2(plg_ramblermailnotify, RamblerMailNotify)
