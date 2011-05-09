#include "ramblermailnotify.h"

#define METAID_MAILNOTIFY   "%1#mail-notify-window"
#define SHC_MAIL_NOTIFY     "/message/x[@xmlns='"NS_RAMBLER_MAIL_NOTIFY"']"

RamblerMailNotify::RamblerMailNotify()
{
	FGateways = NULL;
	FRosterPlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FMetaContacts = NULL;
	FStatusIcons = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FMessageWidgets = NULL;
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
	APluginInfo->homePage = "http://friends.rambler.ru";
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

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
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
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, uchar)),SLOT(onNotificationTest(const QString &, uchar)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
		}
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
		FRostersView->insertClickHooker(RCHO_DEFAULT,this);
	}
	if (FNotifications)
	{
		uchar kindMask = INotification::PopupWindow|INotification::PlaySoundNotification;
		FNotifications->insertNotificator(NID_MAIL_NOTIFY,OWO_NOTIFICATIONS_MAIL_NOTIFY,tr("New e-mail"),kindMask,kindMask);
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

bool RamblerMailNotify::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	if (AOrder==RCHO_DEFAULT && FMailIndexes.contains(AIndex))
	{
		IMetaTabWindow *window = FMetaTabWindows.value(AIndex);
		if (window)
			window->showTabPage();
		else foreach(MailNotifyPage *page, FNotifyPages.values(AIndex))
			page->showTabPage();
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

MailNotify *RamblerMailNotify::findMailNotifyByPopupId(int APopupNotifyId) const
{
	for (QMultiMap<IRosterIndex *, MailNotify *>::const_iterator it = FMailNotifies.begin(); it!=FMailNotifies.end(); it++)
		if (it.value()->popupNotifyId == APopupNotifyId)
			return it.value();
	return NULL;
}

MailNotify *RamblerMailNotify::findMailNotifyByRosterId(int ARosterNotifyId) const
{
	for (QMultiMap<IRosterIndex *, MailNotify *>::const_iterator it = FMailNotifies.begin(); it!=FMailNotifies.end(); it++)
		if (it.value()->rosterNotifyId == ARosterNotifyId)
			return it.value();
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
	MailNotifyPage *page = newMailNotifyPage(AStreamJid,AStanza.from());
	if (page)
	{
		page->appendNewMail(AStanza);
		if (!page->isActive())
		{
			IRosterIndex *mindex = FNotifyPages.key(page);
			QDomElement xElem = AStanza.firstElement("x",NS_RAMBLER_MAIL_NOTIFY);

			MailNotify *mnotify = new MailNotify;
			mnotify->streamJid = page->streamJid();
			mnotify->serviceJid = page->serviceJid();
			mnotify->contactJid = xElem.firstChildElement("contact").text();
			mnotify->pageNotifyId = -1;
			mnotify->popupNotifyId = -1;
			mnotify->rosterNotifyId = -1;

			if (page->tabPageNotifier())
			{
				ITabPageNotify pnotify;
				pnotify.priority = TPNP_NEW_MESSAGE;
				pnotify.iconStorage = RSR_STORAGE_MENUICONS;
				pnotify.iconKey = MNI_RAMBLERMAILNOTIFY_NOTIFY;
				pnotify.count = 1;
				pnotify.toolTip = tr("%n unread","",FMailNotifies.values(mindex).count()+1);
				mnotify->pageNotifyId = page->tabPageNotifier()->insertNotify(pnotify);
			}

			if (FRostersView)
			{
				IRostersNotify rnotify;
				rnotify.order = RNO_RAMBLER_MAIL_NOTIFY;
				rnotify.flags = IRostersNotify::Blink|IRostersNotify::AllwaysVisible;
				rnotify.hookClick = false;
				rnotify.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RAMBLERMAILNOTIFY_NOTIFY);
				rnotify.footer = tr("%n unread","",FMailNotifies.values(mindex).count()+1);
				rnotify.background = QBrush(Qt::yellow);
				mnotify->rosterNotifyId = FRostersView->insertNotify(rnotify,QList<IRosterIndex *>()<<mindex);
			}

			INotification notify;
			notify.kinds = FNotifications!=NULL ? FNotifications->notificatorKinds(NID_MAIL_NOTIFY)|INotification::RosterIcon : 0;
			if ((notify.kinds & (INotification::PopupWindow|INotification::PlaySoundNotification))>0)
			{
				notify.removeInvisible = false;
				notify.notificatior = NID_MAIL_NOTIFY;
				notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
				notify.data.insert(NDR_CONTACT_JID,mnotify->contactJid.full());
				notify.data.insert(NDR_POPUP_CAPTION,tr("New e-mail from"));
				notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
				notify.data.insert(NDR_POPUP_TITLE,xElem.firstChildElement("from").text());
				notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
				notify.data.insert(NDR_POPUP_TEXT,AStanza.firstElement("subject").text());
				notify.data.insert(NDR_SOUND_FILE,SDF_RAMBLERMAILNOTIFY_NOTIFY);

				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
				if (roster && !roster->rosterItem(mnotify->contactJid).isValid)
					notify.data.insert(NDR_POPUP_NOTICE,tr("Not in contact list"));

				mnotify->popupNotifyId = FNotifications->appendNotification(notify);
			}

			FMailNotifies.insertMulti(mindex,mnotify);
		}
	}
}

void RamblerMailNotify::removeMailNotify(MailNotify *ANotify)
{
	IRosterIndex *mindex = ANotify!=NULL ? findMailIndex(ANotify->streamJid) : NULL;
	if (mindex)
	{
		FMailNotifies.remove(mindex,ANotify);
		MailNotifyPage *page = findMailNotifyPage(ANotify->streamJid,ANotify->serviceJid);
		if (page && page->tabPageNotifier())
			page->tabPageNotifier()->removeNotify(ANotify->pageNotifyId);
		if (FRostersView)
			FRostersView->removeNotify(ANotify->rosterNotifyId);
		if (FNotifications)
			FNotifications->removeNotification(ANotify->popupNotifyId);
		delete ANotify;
	}
}

void RamblerMailNotify::clearMailNotifies(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	foreach(MailNotify *mnotify, FMailNotifies.values(mindex))
		removeMailNotify(mnotify);
}

void RamblerMailNotify::clearMailNotifies(MailNotifyPage *APage)
{
	foreach(MailNotify *mnotify, FMailNotifies.values())
		if (mnotify->streamJid==APage->streamJid() && mnotify->serviceJid==APage->serviceJid())
			removeMailNotify(mnotify);
}

MailNotifyPage *RamblerMailNotify::findMailNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	foreach(MailNotifyPage *page, FNotifyPages.values(findMailIndex(AStreamJid)))
		if (page->serviceJid() == AServiceJid)
			return page;
	return NULL;
}

MailNotifyPage *RamblerMailNotify::newMailNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid)
{
	MailNotifyPage *page = findMailNotifyPage(AStreamJid,AServiceJid);
	if (!page)
	{
		IRosterIndex *mindex = findMailIndex(AStreamJid);
		if (mindex)
		{
			page = new MailNotifyPage(FMessageWidgets,mindex,AServiceJid);
			page->setTabPageNotifier(FMessageWidgets!=NULL ? FMessageWidgets->newTabPageNotifier(page) : NULL);
			connect(page->instance(),SIGNAL(showChatWindow(const Jid &)),SLOT(onMailNotifyPageShowChatWindow(const Jid &)));
			connect(page->instance(),SIGNAL(tabPageActivated()),SLOT(onMailNotifyPageActivated()));
			connect(page->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMailNotifyPageDestroyed()));
			FNotifyPages.insertMulti(mindex,page);

			IMetaTabWindow *window = FMetaContacts !=NULL ? FMetaContacts->newMetaTabWindow(AStreamJid,QString(METAID_MAILNOTIFY).arg(AServiceJid.pBare())) : NULL;
			if (window)
			{
				if (!FMetaTabWindows.contains(mindex))
				{
					connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMetaTabWindowDestroyed()));
					FMetaTabWindows.insert(mindex,window);
				}

				// TODO: Find descriptor by service
				IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByOrder(MIO_MAIL);
				QString pageId = window->insertPage(descriptor.metaOrder,false);

				QIcon icon;
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);
				window->setPageIcon(pageId,icon);
				window->setPageName(pageId,tr("New e-mails"));
				window->setPageWidget(pageId,page);
			}
		}
	}
	return page;
}

void RamblerMailNotify::showChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FMessageProcessor)
		FMessageProcessor->createWindow(AStreamJid, AContactJid, Message::Chat, IMessageHandler::SM_SHOW);
}

void RamblerMailNotify::showNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	MailNotifyPage *page = findMailNotifyPage(AStreamJid,AServiceJid);
	if (page)
		page->showTabPage();
}

void RamblerMailNotify::onStreamAdded(const Jid &AStreamJid)
{
	IRosterIndex *sroot = FRostersModel->streamRoot(AStreamJid);
	if (sroot)
	{
		IRosterIndex *mindex = FRostersModel->createRosterIndex(RIT_MAILNOTIFY,sroot);
		mindex->setData(Qt::DisplayRole,tr("Mails"));
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

	IMetaTabWindow *window = FMetaTabWindows.take(mindex);
	if (window)
		delete window->instance();

	foreach(MailNotifyPage *page, FNotifyPages.values(mindex))
	{
		FNotifyPages.remove(mindex,page);
		delete page->instance();
	}

	FMailIndexes.removeAll(mindex);
}

void RamblerMailNotify::onRosterStateChanged(IRoster *ARoster)
{
	if (ARoster->isOpen())
	{
		IRosterIndex *mindex = findMailIndex(ARoster->streamJid());
		foreach(MailNotifyPage *page, FNotifyPages.values(mindex))
			page->clearNewMails();
	}
	clearMailNotifies(ARoster->streamJid());
	updateMailIndex(ARoster->streamJid());
}

void RamblerMailNotify::onNotificationActivated(int ANotifyId)
{
	MailNotify *mnotify = findMailNotifyByPopupId(ANotifyId);
	if (mnotify)
	{
		showChatWindow(mnotify->streamJid, mnotify->contactJid);
		FNotifications->removeNotification(ANotifyId);
	}
}

void RamblerMailNotify::onNotificationRemoved(int ANotifyId)
{
	removeMailNotify(findMailNotifyByPopupId(ANotifyId));
}

void RamblerMailNotify::onNotificationTest(const QString &ANotificatorId, uchar AKinds)
{
	if (ANotificatorId == NID_MAIL_NOTIFY)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.notificatior = ANotificatorId;
		if (AKinds & INotification::PopupWindow)
		{
			notify.data.insert(NDR_POPUP_CAPTION,tr("New e-mail from"));
			notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_TEXT,tr("Hi! Come on mail.rambler.ru :)"));
			notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
		}
		if (AKinds & INotification::PlaySoundNotification)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_RAMBLERMAILNOTIFY_NOTIFY);
		}
		if (!notify.data.isEmpty())
		{
			FNotifications->appendNotification(notify);
		}
	}
}

void RamblerMailNotify::onRosterNotifyActivated(int ANotifyId)
{
	MailNotify *mnotify = findMailNotifyByPopupId(ANotifyId);
	if (mnotify)
	{
		showNotifyPage(mnotify->streamJid,mnotify->serviceJid);
		FRostersView->removeNotify(ANotifyId);
	}
}

void RamblerMailNotify::onRosterNotifyRemoved(int ANotifyId)
{
	removeMailNotify(findMailNotifyByRosterId(ANotifyId));
}

void RamblerMailNotify::onChatWindowCreated(IChatWindow *AWindow)
{
	if (FMetaContacts && FMetaContacts->metaDescriptorByItem(AWindow->contactJid()).gateId==GSID_MAIL)
	{
		MailInfoWidget *widget = new MailInfoWidget(AWindow,AWindow->instance());
		AWindow->insertBottomWidget(CBWO_MAILINFOWIDGET,widget);
	}
}

void RamblerMailNotify::onMailNotifyPageShowChatWindow(const Jid &AContactJid)
{
	MailNotifyPage *page = qobject_cast<MailNotifyPage *>(sender());
	if (page)
		showChatWindow(page->streamJid(),AContactJid);
}

void RamblerMailNotify::onMailNotifyPageActivated()
{
	MailNotifyPage *page = qobject_cast<MailNotifyPage *>(sender());
	if (page)
	{
		clearMailNotifies(page);
	}
}

void RamblerMailNotify::onMailNotifyPageDestroyed()
{
	MailNotifyPage *page = qobject_cast<MailNotifyPage *>(sender());
	if (page)
	{
		clearMailNotifies(page);
		FNotifyPages.remove(FNotifyPages.key(page),page);
	}
}

void RamblerMailNotify::onMetaTabWindowDestroyed()
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	IRosterIndex *mindex = FMetaTabWindows.key(window);
	foreach(ITabPage *page, FNotifyPages.values(mindex))
		delete page->instance();
	FMetaTabWindows.remove(mindex);
}

Q_EXPORT_PLUGIN2(plg_ramblermailnotify, RamblerMailNotify)
