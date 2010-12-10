#include "smsmessagehandler.h"

SmsMessageHandler::SmsMessageHandler()
{

}

SmsMessageHandler::~SmsMessageHandler()
{

}

void SmsMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Sms Messages");
	APluginInfo->description = tr("Allows to exchange sms messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Popov S.A.";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	//APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	//APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	//APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}

bool SmsMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	return true;
}

bool SmsMessageHandler::initObjects()
{
	return true;
}


bool SmsMessageHandler::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	if (/*AMessage.type()==Message::Chat && */!AMessage.body().isEmpty())
		return true;
	return false;
}

bool SmsMessageHandler::showMessage(int AMessageId)
{
	//Message message = FMessageProcessor->messageById(AMessageId);
	//return createWindow(MHO_CHATMESSAGEHANDLER,message.to(),message.from(),Message::Chat,IMessageHandler::SM_SHOW);

#pragma message("SmsMessageHandler::showMessage - не реализован");
	return false;
}

bool SmsMessageHandler::receiveMessage(int AMessageId)
{
	bool notify = false;
	//Message message = FMessageProcessor->messageById(AMessageId);
	//IChatWindow *window = getWindow(message.to(),message.from());
	//if (window)
	//{
	//	StyleExtension extension;
	//	WindowStatus &wstatus = FWindowStatus[window];
	//	if (!window->isActive())
	//	{
	//		notify = true;
	//		extension.extensions = IMessageContentOptions::Unread;
	//		wstatus.notified.append(AMessageId);
	//		updateWindow(window);
	//	}

	//	QUuid contentId = showStyledMessage(window,message,extension);
	//	if (!contentId.isNull() && notify)
	//	{
	//		message.setData(MDR_STYLE_CONTENT_ID,contentId.toString());
	//		wstatus.unread.append(message);
	//	}
	//}
	#pragma message("SmsMessageHandler::receiveMessage - не реализован");
	return notify;
}

INotification SmsMessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
	//IChatWindow *window = getWindow(AMessage.to(),AMessage.from());
	QString name = ANotifications->contactName(AMessage.to(),AMessage.from());
	//QString messages = tr("%n message(s)","",FWindowStatus.value(window).notified.count());

	INotification notify;
	//notify.kinds = ANotifications->notificatorKinds(NOTIFICATOR_ID);
	//notify.data.insert(NDR_STREAM_JID,AMessage.to());
	//notify.data.insert(NDR_CONTACT_JID,AMessage.from());
	//notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
	//notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
	//notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
	//notify.data.insert(NDR_ROSTER_TOOLTIP,messages);
	//notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(messages));
	//notify.data.insert(NDR_TABPAGE_PRIORITY, TPNP_NEW_MESSAGE);
	//notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
	//notify.data.insert(NDR_TABPAGE_TOOLTIP, messages);
	//notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_CHAT_MHANDLER_TABBARITEM_NEWMESSAGE);
	//notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.from()));
	//notify.data.insert(NDR_POPUP_CAPTION,tr("Message received"));
	//notify.data.insert(NDR_POPUP_TITLE,name);
	//notify.data.insert(NDR_POPUP_TEXT,Qt::escape(AMessage.body()));
	//notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);
	//notify.data.insert(NDR_TYPE, NT_CHATMESSAGE);
	#pragma message("SmsMessageHandler::notification - не реализован");

	return notify;
}

bool SmsMessageHandler::createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType == Message::Chat)
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


	#pragma message("SmsMessageHandler::createWindow - не реализован");
	return false;
}

IChatWindow *SmsMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = NULL;
	if (AStreamJid.isValid() && AContactJid.isValid())
	{
		if (AContactJid.resource().isEmpty())
		{
			foreach(IChatWindow *chatWindow, FWindows)
			{
				if (chatWindow->streamJid()==AStreamJid && chatWindow->contactJid().pBare()==AContactJid.pBare())
				{
					window = chatWindow;
					break;
				}
			}
		}
		if (window == NULL)
		{
			window = findWindow(AStreamJid,AContactJid);
		}
		if (window == NULL)
		{
			window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
			if (window)
			{
				window->infoWidget()->autoUpdateFields();
				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				WindowStatus &wstatus = FWindowStatus[window];
				wstatus.createTime = QDateTime::currentDateTime();

				connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
				connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
					SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
				connect(window->viewWidget()->instance(),SIGNAL(urlClicked(const QUrl	&)),SLOT(onUrlClicked(const QUrl	&)));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));

				FWindows.append(window);
				updateWindow(window);

				if (FRostersView && FRostersModel)
				{
					UserContextMenu *menu = new UserContextMenu(FRostersModel,FRostersView,window);
					if (FAvatars)
						FAvatars->insertAutoAvatar(menu->menuAction(),AContactJid,QSize(48,48));
					else
						menu->menuAction()->setIcon(RSR_STORAGE_MENUICONS, MNI_CHAT_MHANDLER_USER_MENU);
					QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
					button->setPopupMode(QToolButton::InstantPopup);
					button->setFixedSize(QSize(48,48));
				}

				setMessageStyle(window);

				TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
				pageInfo.page = window;
				emit tabPageCreated(window);

				requestHistoryMessages(window, HISTORY_MESSAGES_COUNT);

				window->instance()->installEventFilter(this);
			}
		}
	}
	return window;
}



Q_EXPORT_PLUGIN2(plg_smsmessagehandler, SmsMessageHandler)