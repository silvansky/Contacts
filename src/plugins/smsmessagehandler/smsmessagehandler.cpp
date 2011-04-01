#include "smsmessagehandler.h"

#define HISTORY_TIME_PAST         5
#define HISTORY_MESSAGES_COUNT    25

#define WAIT_RECEIVE_TIMEOUT      60
#define DESTROYWINDOW_TIMEOUT     30*60*1000
#define BALANCE_REQUEST_TIMEOUT   30*1000

#define ADR_TAB_PAGE_ID           Action::DR_Parametr2

#define URL_SCHEME_ACTION         "action"
#define URL_PATH_HISTORY          "history"
#define URL_PATH_CONTENT          "content"

#define SMS_DISCO_TYPE            "sms"
#define SMS_DISCO_CATEGORY        "gateway"

#define SHC_SMS_BALANCE           "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SMS_BALANCE "']"
#define SHC_MESSAGE_RECEIPTS      "/message/received[@xmlns='" NS_RECEIPTS "']"

QDataStream &operator<<(QDataStream &AStream, const TabPageInfo &AInfo)
{
	AStream << AInfo.streamJid;
	AStream << AInfo.contactJid;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, TabPageInfo &AInfo)
{
	AStream >> AInfo.streamJid;
	AStream >> AInfo.contactJid;
	AInfo.page = NULL;
	return AStream;
}

SmsMessageHandler::SmsMessageHandler()
{
	FMessageStyles = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FRamblerHistory = NULL;
	FXmppStreams = NULL;
	FDiscovery = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FStanzaProcessor = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;

	FNotReceivedTimer.setInterval(1000);
	connect(&FNotReceivedTimer,SIGNAL(timeout()),SLOT(onNotReceivedTimerTimeout()));
}

SmsMessageHandler::~SmsMessageHandler()
{

}

void SmsMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SMS Messages");
	APluginInfo->description = tr("Allows to exchange SMS messages via gateway");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A.";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SmsMessageHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRamblerHistory").value(0,NULL);
	if (plugin)
	{
		FRamblerHistory = qobject_cast<IRamblerHistory *>(plugin->instance());
		if (FRamblerHistory)
		{
			connect(FRamblerHistory->instance(),SIGNAL(serverMessagesLoaded(const QString &, const IRamblerHistoryMessages &)),
				SLOT(onRamblerHistoryMessagesLoaded(const QString &, const IRamblerHistoryMessages &)));
			connect(FRamblerHistory->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
				SLOT(onRamblerHistoryRequestFailed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FMessageWidgets!=NULL && FMessageProcessor!=NULL && FMessageStyles!=NULL;
}

bool SmsMessageHandler::initObjects()
{
	if (FMessageWidgets)
	{
		FMessageWidgets->insertTabPageHandler(this);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(this,MHO_SMSMESSAGEHANDLER);
	}
	return true;
}

bool SmsMessageHandler::startPlugin()
{
	FNotReceivedTimer.start();
	return true;
}

bool SmsMessageHandler::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISmsBalance.value(AStreamJid) == AHandleId)
	{
		AAccept = true;
		setSmsBalance(AStreamJid,AStanza.from(),smsBalanceFromStanza(AStanza));
		
		Stanza result("iq");
		result.setType("result").setId(AStanza.id()).setTo(AStanza.from());
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	else if (FSHIMessageReceipts.value(AStreamJid) == AHandleId)
	{
		IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AStanza.from()) : NULL;
		if (FWindows.contains(window))
		{
			AAccept = true;
			QString messageId = AStanza.firstElement("received",NS_RECEIPTS).attribute("id");
			replaceRequestedMessage(window,messageId,true);
			return true;
		}
	}
	return false;
}

void SmsMessageHandler::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FSmsBalanceRequests.contains(AStanza.id()))
	{
		Jid serviceJid = FSmsBalanceRequests.take(AStanza.id());
		setSmsBalance(AStreamJid,serviceJid,smsBalanceFromStanza(AStanza));
	}
}

void SmsMessageHandler::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	if (FSmsBalanceRequests.contains(AStanzaId))
	{
		Jid serviceJid = FSmsBalanceRequests.take(AStanzaId);
		setSmsBalance(AStreamJid,serviceJid,-1);
	}
}

bool SmsMessageHandler::tabPageAvail(const QString &ATabPageId) const
{
	if (FTabPages.contains(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		return pageInfo.page!=NULL || findRosterItem(pageInfo.streamJid,pageInfo.contactJid).isValid;
	}
	return false;
}

ITabPage *SmsMessageHandler::tabPageFind(const QString &ATabPageId) const
{
	return FTabPages.contains(ATabPageId) ? FTabPages.value(ATabPageId).page : NULL;
}

ITabPage *SmsMessageHandler::tabPageCreate(const QString &ATabPageId)
{
	ITabPage *page = tabPageFind(ATabPageId);
	if (page==NULL && tabPageAvail(ATabPageId))
	{
		TabPageInfo &pageInfo = FTabPages[ATabPageId];
		IRoster *roster = findRoster(pageInfo.streamJid);
		if (roster)
		{
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(roster->streamJid()) : NULL;
			IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
			if (pitem.isValid)
				page = getWindow(roster->streamJid(), pitem.itemJid);
			else
				page = getWindow(roster->streamJid(), pageInfo.contactJid);
			pageInfo.page = page;
		}
	}
	return page;
}

Action *SmsMessageHandler::tabPageAction(const QString &ATabPageId, QObject *AParent)
{
	if (tabPageAvail(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IRoster *roster = findRoster(pageInfo.streamJid);
		if (roster && roster->isOpen())
		{
			Action *action = new Action(AParent);
			action->setText(pageInfo.contactJid.node());
			action->setData(ADR_TAB_PAGE_ID, ATabPageId);
			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenTabPageAction(bool)));

			ITabPage *page = tabPageFind(ATabPageId);
			if (page)
			{
				if (page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
				{
					ITabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
					if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
						action->setIcon(notify.iconStorage, notify.iconKey);
					else
						action->setIcon(notify.icon);
				}
				else
				{
					action->setIcon(page->tabPageIcon());
				}
			}
			else
			{
				IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(roster->streamJid()) : NULL;
				IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
				if (pitem.isValid)
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pitem.itemJid) : QIcon());
				else
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pageInfo.contactJid.bare()) : QIcon());
			}
			return action;
		}
	}
	return NULL;
}

bool SmsMessageHandler::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	return !AMessage.body().isEmpty() && isSmsContact(AMessage.to(),AMessage.from());
}

bool SmsMessageHandler::showMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	return createWindow(MHO_SMSMESSAGEHANDLER,message.to(),message.from(),Message::Chat,IMessageHandler::SM_SHOW);
}

bool SmsMessageHandler::receiveMessage(int AMessageId)
{
	bool notify = false;
	Message message = FMessageProcessor->messageById(AMessageId);
	IChatWindow *window = getWindow(message.to(),message.from());
	if (window)
	{
		StyleExtension extension;
		WindowStatus &wstatus = FWindowStatus[window];
		if (!window->isActive())
		{
			notify = true;
			extension.extensions = IMessageContentOptions::Unread;
			wstatus.notified.append(AMessageId);
			updateWindow(window);
		}

		QUuid contentId = showStyledMessage(window,message,extension);
		if (!contentId.isNull() && notify)
		{
			message.setData(MDR_STYLE_CONTENT_ID,contentId.toString());
			wstatus.unread.append(message);
		}
	}
	return notify;
}

INotification SmsMessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
	IChatWindow *window = getWindow(AMessage.to(),AMessage.from());
	WindowStatus &wstatus = FWindowStatus[window];

	QString name = ANotifications->contactName(AMessage.to(),AMessage.from());
	QString messages = tr("%n message(s)","",wstatus.notified.count());

	INotification notify;
	notify.kinds = ANotifications->notificatorKinds(NID_CHAT_MESSAGE);
	if (notify.kinds > 0)
	{
		notify.notificatior = NID_CHAT_MESSAGE;
		notify.data.insert(NDR_STREAM_JID,AMessage.to());
		notify.data.insert(NDR_CONTACT_JID,AMessage.from());
		notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_ORDER,RNO_CHAT_MHANDLER_MESSAGE);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
		notify.data.insert(NDR_ROSTER_HOOK_CLICK,true);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
		notify.data.insert(NDR_ROSTER_FOOTER,messages);
		notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::yellow));
		notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(messages));
		notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_NEW_MESSAGE);
		notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,wstatus.notified.count());
		notify.data.insert(NDR_TABPAGE_CREATE_TAB,true);
		notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
		notify.data.insert(NDR_TABPAGE_TOOLTIP,messages);
		notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_CHAT_MHANDLER_TABBARITEM_NEWMESSAGE);
		notify.data.insert(NDR_POPUP_CAPTION,tr("Writing..."));
		notify.data.insert(NDR_POPUP_TITLE,name);
		notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.from()));
		notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);

		if (wstatus.notified.count() > 1)
		{
			QList<int> notifies = ANotifications->notifications();
			int replNotify = FMessageProcessor->notifyByMessage(wstatus.notified.value(wstatus.notified.count()-2));
			if (!notifies.isEmpty() && notifies.last()==replNotify)
				notify.data.insert(NDR_REPLACE_NOTIFY, replNotify);
		}

		QTextDocument doc;
		FMessageProcessor->messageToText(&doc,AMessage);
		notify.data.insert(NDR_POPUP_TEXT,getHtmlBody(doc.toHtml()));
	}
	return notify;
}

bool SmsMessageHandler::createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType==Message::Chat && isSmsContact(AStreamJid,AContactJid))
	{
		IChatWindow *window = getWindow(AStreamJid,AContactJid);
		if (window)
		{
			if (AShowMode==IMessageHandler::SM_SHOW)
				window->showTabPage();
			else if (!window->instance()->isVisible() && AShowMode==IMessageHandler::SM_ADD_TAB)
				FMessageWidgets->assignTabWindowPage(window);
			return true;
		}
	}
	return false;
}

bool SmsMessageHandler::isSmsContact(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (!AContactJid.node().isEmpty() && AContactJid.pDomain().endsWith("."+AStreamJid.pDomain()))
	{
		if (FDiscovery && FDiscovery->hasDiscoInfo(AStreamJid,AContactJid.domain()))
		{
			IDiscoIdentity ident = FDiscovery->discoInfo(AStreamJid,AContactJid.domain()).identity.value(0);
			return ident.category==SMS_DISCO_CATEGORY && ident.type==SMS_DISCO_TYPE;
		}
		return AContactJid.pDomain().startsWith("sms.");
	}
	return false;
}

int SmsMessageHandler::smsBalance(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	return FSmsBalance.value(AStreamJid).value(AServiceJid,-1);
}

bool SmsMessageHandler::requestSmsBalance(const Jid &AStreamJid, const Jid &AServiceJid)
{
	if (FStanzaProcessor)
	{
		Stanza request("iq");
		request.setType("get").setId(FStanzaProcessor->newId()).setTo(AServiceJid.eBare());
		request.addElement("query",NS_RAMBLER_SMS_BALANCE);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,BALANCE_REQUEST_TIMEOUT))
		{
			FSmsBalanceRequests.insert(request.id(),AServiceJid);
			return true;
		}
	}
	return false;
}

int SmsMessageHandler::smsBalanceFromStanza(const Stanza &AStanza) const
{
	QDomElement balanceElem = AStanza.firstElement("query",NS_RAMBLER_SMS_BALANCE).firstChildElement("balance");
	return !balanceElem.isNull() ? balanceElem.text().toInt() : -1;
}

void SmsMessageHandler::setSmsBalance(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance)
{
	if (FSmsBalance.contains(AStreamJid))
	{
		if (ABalance >= 0)
			FSmsBalance[AStreamJid].insert(AServiceJid,ABalance);
		else
			FSmsBalance[AStreamJid].remove(AServiceJid);
		emit smsBalanceChanged(AStreamJid,AServiceJid,ABalance);
	}
}

IRoster *SmsMessageHandler::findRoster(const Jid &AStreamJid) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	for (int i=0; roster==NULL && i<FRosters.count(); i++)
		if (AStreamJid && FRosters.at(i)->streamJid())
			roster = FRosters.at(i);
	return roster;
}

IRosterItem SmsMessageHandler::findRosterItem(const Jid &AStreamJid, const Jid &AContactJid) const
{
	IRoster *roster = findRoster(AStreamJid);
	return roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
}

IPresenceItem SmsMessageHandler::findPresenceItem(IPresence *APresence, const Jid &AContactJid) const
{
	IPresenceItem pitem = APresence!=NULL ? APresence->presenceItem(AContactJid) : IPresenceItem();
	QList<IPresenceItem> pitems = APresence!=NULL ? APresence->presenceItems() : QList<IPresenceItem>();
	for (int i=0; !pitem.isValid && i<pitems.count(); i++)
		if (AContactJid && pitems.at(i).itemJid)
			pitem = pitems.at(i);
	return pitem;
}

IChatWindow *SmsMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = findWindow(AStreamJid,AContactJid);
	if (!window)
	{
		window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
		if (window)
		{
			window->infoWidget()->autoUpdateFields();
			window->editWidget()->setSendKey(QKeySequence::UnknownKey);
			window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

			WindowStatus &wstatus = FWindowStatus[window];
			wstatus.createTime = QDateTime::currentDateTime();

			connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
			connect(window->viewWidget()->instance(),SIGNAL(urlClicked(const QUrl	&)),SLOT(onUrlClicked(const QUrl	&)));
			connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
			connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));

			FWindows.append(window);
			updateWindow(window);
			setMessageStyle(window);

			SmsInfoWidget *infoWidget = new SmsInfoWidget(this, window, window->instance());
			window->insertBottomWidget(CBWO_SMSINFOWIDGET,infoWidget);

			TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
			pageInfo.page = window;
			emit tabPageCreated(window);

			requestHistoryMessages(window, HISTORY_MESSAGES_COUNT);

			window->instance()->installEventFilter(this);
		}
	}
	return window;
}

IChatWindow *SmsMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	foreach(IChatWindow *window, FWindows)
		if (window->streamJid()==AStreamJid && window->contactJid()==AContactJid)
			return window;
	return NULL;
}

void SmsMessageHandler::clearWindow(IChatWindow *AWindow)
{
	IMessageStyle *style = AWindow->viewWidget()!=NULL ? AWindow->viewWidget()->messageStyle() : NULL;
	if (style!=NULL)
	{
		IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
		style->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true);
		resetWindowStatus(AWindow);
	}
}

void SmsMessageHandler::updateWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (!FWindowStatus.value(AWindow).notified.isEmpty())
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
	else if (FStatusIcons)
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString name = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
	QString show = FStatusChanger!=NULL ? FStatusChanger->nameByShow(AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt()) : QString::null;
	QString title = name + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);
	AWindow->updateWindow(icon,name,title,show);
}

void SmsMessageHandler::resetWindowStatus( IChatWindow *AWindow )
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	wstatus.separators.clear();
	wstatus.unread.clear();
	wstatus.offline.clear();
	wstatus.historyId = QString::null;
	wstatus.historyTime = QDateTime();
	wstatus.historyRequestId = QUuid();
	wstatus.lastStatusShow = QString::null;
}

void SmsMessageHandler::removeMessageNotifications(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.notified.isEmpty())
	{
		foreach(int messageId, wstatus.notified)
			FMessageProcessor->removeMessage(messageId);
		wstatus.notified.clear();
		updateWindow(AWindow);
	}
}

void SmsMessageHandler::replaceUnreadMessages(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.unread.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		foreach (Message message, wstatus.unread)
		{
			extension.contentId = message.data(MDR_STYLE_CONTENT_ID).toString();
			showStyledMessage(AWindow, message, extension);
		}
		wstatus.unread.clear();
	}
}

void SmsMessageHandler::replaceRequestedMessage(IChatWindow *AWindow, const QString &AMessageId, bool AReceived)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.requested.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		for(int i=0; i<wstatus.requested.count(); i++)
		{
			Message message = wstatus.requested.at(i);
			if (message.id() == AMessageId)
			{
				extension.notice = !AReceived ? tr("SMS not sent!") : QString::null;
				extension.contentId = message.data(MDR_STYLE_CONTENT_ID).toString();
				showStyledMessage(AWindow, message, extension);
				wstatus.requested.removeAt(i);
				break;
			}
		}
	}
}

void SmsMessageHandler::requestHistoryMessages(IChatWindow *AWindow, int ACount)
{
	if (FRamblerHistory && FRamblerHistory->isSupported(AWindow->streamJid()))
	{
		IRamblerHistoryRetrieve retrieve;
		retrieve.with = AWindow->contactJid();
		retrieve.beforeId = FWindowStatus.value(AWindow).historyId;
		retrieve.beforeTime = FWindowStatus.value(AWindow).historyTime;
		retrieve.count = ACount;
		QString id = FRamblerHistory->loadServerMessages(AWindow->streamJid(),retrieve);
		if (!id.isEmpty())
		{
			FHistoryRequests.insert(id,AWindow);
			showHistoryLinks(AWindow,HLS_WAITING);
		}
	}
}

void SmsMessageHandler::showHistoryLinks(IChatWindow *AWindow, HisloryLoadState AState, bool AInit)
{
	static QString urlMask = QString("<a href='%1'>%2</a>");
	if (FRamblerHistory && FRamblerHistory->isSupported(AWindow->streamJid()))
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.time = QDateTime::fromTime_t(0);
		options.timeFormat = " ";
		options.noScroll = true;

		if (AInit && false)
		{
			QUrl showWindowUrl;
			showWindowUrl.setScheme(URL_SCHEME_ACTION);
			showWindowUrl.setPath(URL_PATH_HISTORY);
			showWindowUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("window")));
			options.status = IMessageContentOptions::HistoryShow;
			AWindow->viewWidget()->changeContentHtml(urlMask.arg(showWindowUrl.toString()).arg(tr("SMS history")),options);
		}

		QString message;
		options.status = IMessageContentOptions::HistoryRequest;
		if (AState == HLS_READY)
		{
			QUrl showMesagesUrl;
			showMesagesUrl.setScheme(URL_SCHEME_ACTION);
			showMesagesUrl.setPath(URL_PATH_HISTORY);
			showMesagesUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("messages")));
			message = urlMask.arg(showMesagesUrl.toString()).arg(tr("Show previous messages"));
		}
		else if (AState == HLS_WAITING)
		{
			message = tr("Loading messages from server...");
		}

		WindowStatus &wstatus = FWindowStatus[AWindow];
		if (!wstatus.historyRequestId.isNull())
		{
			options.action = AState!=HLS_FINISHED ? IMessageContentOptions::Replace : IMessageContentOptions::Remove;
			options.contentId = wstatus.historyRequestId;
		}
		wstatus.historyRequestId =  AWindow->viewWidget()->changeContentHtml(message,options);
	}
}

void SmsMessageHandler::setMessageStyle(IChatWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
	IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
	AWindow->viewWidget()->setMessageStyle(style,soptions);
	resetWindowStatus(AWindow);
	showHistoryLinks(AWindow, HLS_READY, true);
}

void SmsMessageHandler::fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
	if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		AOptions.senderId = AWindow->contactJid().full();
		AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid(),AWindow->contactJid()));
		AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->contactJid());
		AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid(),AWindow->contactJid());
		AOptions.senderColor = "blue";
	}
	else
	{
		AOptions.senderId = AWindow->streamJid().full();
		if (AWindow->streamJid() && AWindow->contactJid())
			AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().node());
		else
			AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid()));
		AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->streamJid());
		AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid());
		AOptions.senderColor = "red";
	}
}

QUuid SmsMessageHandler::showDateSeparator(IChatWindow *AWindow, const QDate &ADate)
{
	static const QList<QString> mnames = QList<QString>() << tr("January") << tr("February") <<  tr("March") <<  tr("April")
		<< tr("May") << tr("June") << tr("July") << tr("August") << tr("September") << tr("October") << tr("November") << tr("December");
	static const QList<QString> dnames = QList<QString>() << tr("Monday") << tr("Tuesday") <<  tr("Wednesday") <<  tr("Thursday")
		<< tr("Friday") << tr("Saturday") << tr("Sunday");

	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.separators.contains(ADate))
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.status = IMessageContentOptions::DateSeparator;
		options.direction = IMessageContentOptions::DirectionIn;
		options.time.setDate(ADate);
		options.time.setTime(QTime(0,0));
		options.timeFormat = " ";
		options.noScroll = true;

		QString message;
		QDate currentDate = QDate::currentDate();
		if (ADate == currentDate)
			message = ADate.toString(tr("%1, %2 dd")).arg(tr("Today")).arg(mnames.value(ADate.month()-1));
		else if (ADate.year() == currentDate.year())
			message = ADate.toString(tr("%1, %2 dd")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		else
			message = ADate.toString(tr("%1, %2 dd, yyyy")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		wstatus.separators.append(ADate);
		return AWindow->viewWidget()->changeContentText(message,options);
	}
	return QUuid();
}

QUuid SmsMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Status;
	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	fillContentOptions(AWindow,options);
	return AWindow->viewWidget()->changeContentText(AMessage,options);
}

QUuid SmsMessageHandler::showStyledMessage(IChatWindow *AWindow, const Message &AMessage, const StyleExtension &AExtension)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Message;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid() != AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionOut;

	if (options.time.secsTo(FWindowStatus.value(AWindow).createTime) > HISTORY_TIME_PAST)
	{
		options.noScroll = true;
		options.type |= IMessageContentOptions::History;
	}

	options.action = AExtension.action;
	options.extensions = AExtension.extensions;
	options.contentId = AExtension.contentId;
	options.notice = AExtension.notice;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,AMessage.dateTime().date());
	return AWindow->viewWidget()->changeContentMessage(AMessage,options);
}

bool SmsMessageHandler::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type()==QEvent::WindowDeactivate || AEvent->type()==QEvent::Hide)
	{
		IChatWindow *window = qobject_cast<IChatWindow *>(AObject);
		if (window)
			replaceUnreadMessages(window);
	}
	return QObject::eventFilter(AObject,AEvent);
}

void SmsMessageHandler::onMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		Message message;
		message.setFrom(window->streamJid().eFull()).setTo(window->contactJid().eFull()).setType(Message::Chat).setId(FStanzaProcessor->newId());
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		message.stanza().addElement("request",NS_RECEIPTS);
		if (!message.body().trimmed().isEmpty() && FMessageProcessor->sendMessage(window->streamJid(),message))
		{
			StyleExtension extension;
			extension.notice = tr("Sending...");
			QUuid contentId = showStyledMessage(window, message, extension);
			if (!contentId.isNull())
			{
				message.setData(MDR_STYLE_CONTENT_ID, contentId.toString());
				message.setData(MDR_SMS_REQUEST_TIME,QDateTime::currentDateTime());
				FWindowStatus[window].requested.append(message);
			}
			replaceUnreadMessages(window);
			window->editWidget()->clearEditor();
		}
	}
}

void SmsMessageHandler::onUrlClicked(const QUrl &AUrl)
{
	if (AUrl.scheme() == URL_SCHEME_ACTION)
	{
		IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			if (AUrl.path() == URL_PATH_HISTORY)
			{
				QString keyValue = AUrl.queryItemValue("show");
				if (keyValue == "messages")
				{
					requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
				}
				//else if (keyValue == "window")
				//{

				//}
			}
			//else if (AUrl.path() == URL_PATH_CONTENT)
			//{
			//	QUuid contentId = AUrl.queryItemValue("remove");
			//	if (!contentId.isNull())
			//	{
			//		removeOfflineMessage(window,contentId);
			//	}
			//}
		}
	}
}

void SmsMessageHandler::onWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
		pageInfo.streamJid = window->streamJid();
		pageInfo.contactJid = window->contactJid();
		pageInfo.page = window;

		if (FWindowTimers.contains(window))
			delete FWindowTimers.take(window);
		removeMessageNotifications(window);
	}
}

void SmsMessageHandler::onWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		if (!FWindowTimers.contains(window))
		{
			QTimer *timer = new QTimer;
			timer->setSingleShot(true);
			connect(timer,SIGNAL(timeout()),window->instance(),SLOT(deleteLater()));
			FWindowTimers.insert(window,timer);
		}
		FWindowTimers[window]->start(DESTROYWINDOW_TIMEOUT);
	}
}

void SmsMessageHandler::onWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		if (FTabPages.contains(window->tabPageId()))
			FTabPages[window->tabPageId()].page = NULL;
		if (FWindowTimers.contains(window))
			delete FWindowTimers.take(window);
		removeMessageNotifications(window);
		FWindows.removeAll(window);
		FWindowStatus.remove(window);
		emit tabPageDestroyed(window);
	}
}

void SmsMessageHandler::onStatusIconsChanged()
{
	foreach(IChatWindow *window, FWindows)
		updateWindow(window);
}

void SmsMessageHandler::onOpenTabPageAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		ITabPage *page = tabPageCreate(action->data(ADR_TAB_PAGE_ID).toString());
		if (page)
			page->showTabPage();
	}
}

void SmsMessageHandler::onNotReceivedTimerTimeout()
{
	QDateTime curDT = QDateTime::currentDateTime();
	for (QMap<IChatWindow *, WindowStatus>::iterator it = FWindowStatus.begin(); it!=FWindowStatus.end(); it++)
	{
		for(int i=0; i<it->requested.count(); i++)
		{
			if (it->requested.at(i).data(MDR_SMS_REQUEST_TIME).toDateTime().secsTo(curDT) > WAIT_RECEIVE_TIMEOUT)
			{
				replaceRequestedMessage(it.key(),it->requested.at(i).id(),false);
				i--;
			}
		}
	}
}

void SmsMessageHandler::onRamblerHistoryMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			for (int i=0; i<AMessages.messages.count(); i++)
			{
				Message message = AMessages.messages.at(i);
				showStyledMessage(window,message);
			}

			if (!AMessages.beforeId.isEmpty())
			{
				FWindowStatus[window].historyId = AMessages.beforeId;
				FWindowStatus[window].historyTime = AMessages.beforeTime;
			}

			if (AMessages.messages.count() < HISTORY_MESSAGES_COUNT)
				showHistoryLinks(window,HLS_FINISHED);
			else
				showHistoryLinks(window,HLS_READY);
		}
	}
}

void SmsMessageHandler::onRamblerHistoryRequestFailed(const QString &AId, const QString &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			IMessageContentOptions options;
			options.kind = IMessageContentOptions::Status;
			options.type |= IMessageContentOptions::Event;
			options.direction = IMessageContentOptions::DirectionIn;
			options.time = QDateTime::currentDateTime();
			window->viewWidget()->changeContentText(tr("Failed to load history messages from server: %1").arg(AError),options);
			showHistoryLinks(window,HLS_FINISHED);
		}
	}
}

void SmsMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach(IChatWindow *window, FWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setMessageStyle(window);
				requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
			}
		}
	}
}

void SmsMessageHandler::onPresenceOpened(IPresence *APresence)
{
	foreach(IChatWindow *window, FWindows)
	{
		if (window->streamJid() == APresence->streamJid())
		{
			if (FRamblerHistory && FRamblerHistory->isSupported(window->streamJid()))
			{
				clearWindow(window);
				requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
			}
		}
	}
}

void SmsMessageHandler::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_DEFAULT;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.streamJid = AXmppStream->streamJid();
		handle.conditions.append(SHC_SMS_BALANCE);
		FSHISmsBalance.insert(AXmppStream->streamJid(),FStanzaProcessor->insertStanzaHandle(handle));

		handle.order = SHO_MI_SMSRECEIPTS;
		handle.conditions.clear();
		handle.conditions.append(SHC_MESSAGE_RECEIPTS);
		FSHIMessageReceipts.insert(AXmppStream->streamJid(),FStanzaProcessor->insertStanzaHandle(handle));
	}
	FSmsBalance[AXmppStream->streamJid()].clear();
}

void SmsMessageHandler::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	QMap<Jid, int> balances = FSmsBalance.take(AXmppStream->streamJid());
	for (QMap<Jid, int>::const_iterator it = balances.constBegin(); it!=balances.constEnd(); it++)
	{
		emit smsBalanceChanged(AXmppStream->streamJid(),it.key(),-1);
	}
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHISmsBalance.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessageReceipts.take(AXmppStream->streamJid()));
	}
}

void SmsMessageHandler::onRosterAdded(IRoster *ARoster)
{
	FRosters.append(ARoster);
}

void SmsMessageHandler::onRosterRemoved(IRoster *ARoster)
{
	FRosters.removeAll(ARoster);
}

void SmsMessageHandler::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.last-sms-tab-pages").toByteArray();
	QDataStream stream(data);
	stream >> FTabPages;
}

void SmsMessageHandler::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FTabPages;
	Options::setFileValue(data,"messages.last-sms-tab-pages");
}

Q_EXPORT_PLUGIN2(plg_smsmessagehandler, SmsMessageHandler)
