#include "ramblermailnotify.h"

#define METAID_MAILNOTIFY   "%1#mail-notify-window"
#define SHC_MAIL_NOTIFY     "/message/x[@xmlns='rambler:mail:notice']"

RamblerMailNotify::RamblerMailNotify()
{
	FGateways = NULL;
	FXmppStreams = NULL;
	FRosterPlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FMetaContacts = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FDiscovery = NULL;

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
	APluginInfo->homePage = "http://contacts.rambler.ru";
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

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
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
			connect(FRostersModel->instance(),SIGNAL(streamRemoved(const Jid &)),SLOT(onRosterModelStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, ushort)),SLOT(onNotificationTest(const QString &, ushort)));
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

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
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
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_MAIL_NOTIFY;
		notifyType.title = tr("New e-mail");
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_MAIL_NOTIFY,notifyType);
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
		if (FGateways && FGateways->streamServices(AStreamJid).contains(AStanza.from()))
		{
			AAccept = true;
			LogDetaile(QString("[RamblerMailNotify] Rambler mail notify received from '%1'").arg(AStanza.from()));
			insertMailNotify(AStreamJid,AStanza);
		}
		else if (FGateways)
		{
			LogError(QString("[RamblerMailNotify] Rambler mail notify received from not stream service '%1'").arg(AStanza.from()));
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

IRosterIndex *RamblerMailNotify::getMailIndex(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (!mindex)
	{
		IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
		if (sroot)
		{
			mindex = FRostersModel->createRosterIndex(RIT_MAILNOTIFY,sroot);
			mindex->setData(Qt::DisplayRole,tr("Mails"));
			mindex->setData(RDR_TYPE_ORDER,RITO_MAILNOTIFY);
			mindex->setData(RDR_AVATAR_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
			if (FRostersView)
				FRostersView->insertLabel(FAvatarLabelId,mindex);
			FMailIndexes.append(mindex);
			FRostersModel->insertRosterIndex(mindex,sroot);
			updateMailIndex(AStreamJid);
		}
	}
	return mindex;
}

void RamblerMailNotify::updateMailIndex(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (mindex)
	{
		int mails = 0;
		foreach(MailNotifyPage *page, FNotifyPages.values(mindex))
			mails += page->newMailsCount();
		QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RAMBLERMAILNOTIFY_ROSTER,mails>0 ? 1 : 0);
		if (FRostersView)
			FRostersView->insertFooterText(FTO_ROSTERSVIEW_STATUS,mails>0 ? tr("%n new mail(s)","",mails) : tr("No new mails"),mindex);
		mindex->setData(Qt::DecorationRole, icon);
	}
}

void RamblerMailNotify::removeMailIndex(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (mindex)
	{
		IMetaTabWindow *window = FMetaTabWindows.take(mindex);
		if (window)
			delete window->instance();

		foreach(MailNotifyPage *page, FNotifyPages.values(mindex))
		{
			FNotifyPages.remove(mindex,page);
			delete page->instance();
		}

		foreach(CustomMailPage *page, FCustomPages.values(mindex))
		{
			FCustomPages.remove(mindex,page);
			delete page->instance();
		}

		clearMailNotifies(AStreamJid);
		FMailIndexes.removeAll(mindex);
		mindex->instance()->deleteLater();
	}
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

void RamblerMailNotify::insertMailNotify(const Jid &AStreamJid, const Stanza &AStanza)
{
	MailNotifyPage *page = getMailNotifyPage(AStreamJid,AStanza.from());
	if (page)
	{
		page->appendNewMail(AStanza);
		if (!page->isActiveTabPage())
		{
			IRosterIndex *mindex = FNotifyPages.key(page);
			QDomElement contactElem = AStanza.firstElement("x",NS_RAMBLER_MAIL_NOTICE).firstChildElement("contact");

			MailNotify *mnotify = new MailNotify;
			mnotify->streamJid = page->streamJid();
			mnotify->serviceJid = page->serviceJid();
			mnotify->contactJid = contactElem.firstChildElement("jid").text();
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
			notify.kinds = FNotifications!=NULL ? FNotifications->notificationKinds(NNT_MAIL_NOTIFY)|INotification::RosterNotify : 0;
			if ((notify.kinds & (INotification::PopupWindow|INotification::SoundPlay))>0)
			{
				notify.typeId = NNT_MAIL_NOTIFY;
				notify.flags &= ~INotification::RemoveInvisible;
				notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
				notify.data.insert(NDR_CONTACT_JID,mnotify->contactJid.full());
				notify.data.insert(NDR_POPUP_TITLE,contactElem.firstChildElement("name").text());
				notify.data.insert(NDR_POPUP_NOTICE,tr("New e-mail"));
				notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
				notify.data.insert(NDR_POPUP_TEXT,AStanza.firstElement("subject").text());
				notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
				notify.data.insert(NDR_SOUND_FILE,SDF_RAMBLERMAILNOTIFY_NOTIFY);

				//IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
				//if (roster && !roster->rosterItem(mnotify->contactJid).isValid)
				//	notify.data.insert(NDR_POPUP_NOTICE,tr("Not in contact list"));

				mnotify->popupNotifyId = FNotifications->appendNotification(notify);
			}

			FMailNotifies.insertMulti(mindex,mnotify);
			updateMailIndex(AStreamJid);
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

MailNotifyPage *RamblerMailNotify::getMailNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid)
{
	MailNotifyPage *page = findMailNotifyPage(AStreamJid,AServiceJid);
	if (!page && FMessageWidgets)
	{
		IRosterIndex *mindex = getMailIndex(AStreamJid);
		if (mindex)
		{
			page = new MailNotifyPage(FMessageWidgets,mindex,AServiceJid);
			page->setTabPageNotifier(FMessageWidgets!=NULL ? FMessageWidgets->newTabPageNotifier(page) : NULL);
			connect(page->instance(),SIGNAL(showCustomMailPage()),SLOT(onMainNotifyPageShowCustomPage()));
			connect(page->instance(),SIGNAL(showChatWindow(const Jid &)),SLOT(onMailNotifyPageShowChatWindow(const Jid &)));
			connect(page->instance(),SIGNAL(tabPageActivated()),SLOT(onMailNotifyPageActivated()));
			connect(page->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMailNotifyPageDestroyed()));
			FNotifyPages.insertMulti(mindex,page);

			/*IMetaTabWindow *window = FMetaContacts !=NULL ? FMetaContacts->newMetaTabWindow(AStreamJid,QString(METAID_MAILNOTIFY).arg(AStreamJid.pBare())) : NULL;
			if (window)
			{
				if (!FMetaTabWindows.contains(mindex))
				{
					connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMetaTabWindowDestroyed()));
					FMetaTabWindows.insert(mindex,window);
				}

				IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(AServiceJid);
				QString pageId = window->insertPage(descriptor.metaOrder,false);

				QIcon icon;
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
				icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);
				window->setPageIcon(pageId,icon);
				window->setPageName(pageId,page->tabPageCaption());
				window->setPageWidget(pageId,page);
			}*/
		}
	}
	return page;
}

CustomMailPage *RamblerMailNotify::findCustomMailPage(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	foreach(CustomMailPage *page, FCustomPages.values(findMailIndex(AStreamJid)))
		if (page->serviceJid() == AServiceJid)
			return page;
	return NULL;
}

CustomMailPage *RamblerMailNotify::getCustomMailPage(const Jid &AStreamJid, const Jid &AServiceJid)
{
	CustomMailPage *page = findCustomMailPage(AStreamJid,AServiceJid);
	if (!page && FGateways && FMessageWidgets)
	{
		IRosterIndex *mindex = getMailIndex(AStreamJid);
		if (mindex)
		{
			page = new CustomMailPage(FGateways,FMessageWidgets,mindex,AServiceJid);
			connect(page->instance(),SIGNAL(showChatWindow(const Jid &)),SLOT(onCustomMailPageShowChatWindow(const Jid &)));
			connect(page->instance(),SIGNAL(tabPageDestroyed()),SLOT(onCustomMailPageDestroyed()));
			FCustomPages.insertMulti(mindex,page);
		}
	}
	return page;
}

void RamblerMailNotify::showChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FMessageProcessor)
		FMessageProcessor->createMessageWindow(AStreamJid, AContactJid, Message::Chat, IMessageHandler::SM_SHOW);
}

void RamblerMailNotify::showNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	MailNotifyPage *page = findMailNotifyPage(AStreamJid,AServiceJid);
	if (page)
		page->showTabPage();
}

void RamblerMailNotify::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	removeMailIndex(AXmppStream->streamJid());
}

void RamblerMailNotify::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	clearMailNotifies(AXmppStream->streamJid());
}

void RamblerMailNotify::onRosterModelStreamRemoved(const Jid &AStreamJid)
{
	removeMailIndex(AStreamJid);
}

void RamblerMailNotify::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.features.contains(NS_RAMBLER_MAIL_NOTIFY))
	{
		if (FGateways && FGateways->streamServices(AInfo.streamJid).contains(AInfo.contactJid))
		{
			getMailNotifyPage(AInfo.streamJid,AInfo.contactJid);
		}
	}
}

void RamblerMailNotify::onNotificationActivated(int ANotifyId)
{
	MailNotify *mnotify = findMailNotifyByPopupId(ANotifyId);
	if (mnotify)
	{
		showNotifyPage(mnotify->streamJid,mnotify->contactJid.domain());
		FNotifications->removeNotification(ANotifyId);
	}
}

void RamblerMailNotify::onNotificationRemoved(int ANotifyId)
{
	removeMailNotify(findMailNotifyByPopupId(ANotifyId));
}

void RamblerMailNotify::onNotificationTest(const QString &ATypeId, ushort AKinds)
{
	if (ATypeId == NNT_MAIL_NOTIFY)
	{
		INotification notify;
		notify.typeId = ATypeId;
		notify.kinds = AKinds;
		notify.flags |= INotification::TestNotify;
		if (AKinds & INotification::PopupWindow)
		{
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_NOTICE,tr("New e-mail"));
			notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_RAMBLERMAILNOTIFY_AVATAR));
			notify.data.insert(NDR_POPUP_TEXT,tr("Hi! Come on mail.rambler.ru :)"));
			notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
		}
		if (AKinds & INotification::SoundPlay)
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

void RamblerMailNotify::onMainNotifyPageShowCustomPage()
{
	MailNotifyPage *page = qobject_cast<MailNotifyPage *>(sender());
	if (page)
	{
		CustomMailPage *customPage = getCustomMailPage(page->streamJid(),page->serviceJid());
		if (customPage)
			customPage->showTabPage();
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
		clearMailNotifies(page);
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

void RamblerMailNotify::onCustomMailPageShowChatWindow(const Jid &AContactJid)
{
	CustomMailPage *page = qobject_cast<CustomMailPage *>(sender());
	if (page)
		showChatWindow(page->streamJid(),AContactJid);
}

void RamblerMailNotify::onCustomMailPageDestroyed()
{
	CustomMailPage *page = qobject_cast<CustomMailPage *>(sender());
	if (page)
		FCustomPages.remove(FCustomPages.key(page),page);
}

Q_EXPORT_PLUGIN2(plg_ramblermailnotify, RamblerMailNotify)
