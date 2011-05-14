#include "sipphone.h"
#include "sipcallnotifyer.h"
#include <QMessageBox>
#include <utils/log.h>
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>

#include "winsock2.h"


#define SHC_SIP_REQUEST "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SIP_PHONE "']"

#define ADR_STREAM_JID    Action::DR_StreamJid
#define ADR_CONTACT_JID   Action::DR_Parametr1
#define ADR_STREAM_ID     Action::DR_Parametr2
#define ADR_METAID_WINDOW Action::DR_Parametr3

#define CLOSE_TIMEOUT   10000
#define REQUEST_TIMEOUT 30000

SipPhone::SipPhone()
{
	FDiscovery = NULL;
	FMetaContacts = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
	FNotifications = NULL;
	//FRostersViewPlugin = NULL;
	FPresencePlugin = NULL;

	FSHISipRequest = -1;

	FSipPhoneProxy = NULL;

	connect(this, SIGNAL(streamStateChanged(const QString&, int)), this, SLOT(onStreamStateChanged(const QString&, int)));
}

SipPhone::~SipPhone()
{
	if(FSipPhoneProxy != NULL)
	{
		delete FSipPhoneProxy;
		FSipPhoneProxy = NULL;
	}
}

void SipPhone::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SIP Phone");
	APluginInfo->description = tr("Allows to make voice and video calls over SIP protocol");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Popov S.A.";
	APluginInfo->homePage = "http://friends.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SipPhone::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		if(FMetaContacts)
		{
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowCreated(IMetaTabWindow*)), SLOT(onMetaTabWindowCreated(IMetaTabWindow*)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{

		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
			connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}

		//FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		//if (FRostersViewPlugin)
		//{
		//	connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
		//		SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
		//	connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
		//		SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		//}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *)));
		//connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
	}


#ifdef WIN32
	WSADATA ws;
	if(FAILED(WSAStartup(MAKEWORD(2, 2), &ws)))
	{
		int error = WSAGetLastError();
		Log(QString("[WSAStartup error] code %1").arg(error));
		exit(1);
	}
#endif
	SipProtoInit::Init();
	VoIPMediaInit::Init();
	SipProtoInit::SetListenSipPort(5060);
	SipProtoInit::SetProxySipPort(5060);

	return FStanzaProcessor!=NULL;
}

bool SipPhone::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipPhone;
		sipPhone.active = true;
		sipPhone.var = NS_RAMBLER_SIP_PHONE;
		sipPhone.name = tr("SIP Phone");
		sipPhone.description = tr("SIP voice and video calls");
		FDiscovery->insertDiscoFeature(sipPhone);
	}
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_SIP_REQUEST);
		FSHISipRequest = FStanzaProcessor->insertStanzaHandle(shandle);
	}
	if (FNotifications)
	{
		uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySoundNotification;
		uchar kindDefs = INotification::RosterIcon|INotification::PopupWindow|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySoundNotification;
		FNotifications->insertNotificator(NID_SIPPHONE_CALL,OWO_NOTIFICATIONS_SIPPHONE,QString::null,kindMask,kindDefs);
	}

//	SipCallNotifyer * callNotifyer = new SipCallNotifyer("Tester", tr("Incoming call"), IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL), FNotifications->contactAvatar(Jid()));
//	callNotifyer->appear();

	return true;
}


void SipPhone::onStreamOpened(IXmppStream * AXmppStream)
{
	QString hostAddress;
	IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(AXmppStream->connection()->instance());
	if (defConnection)
	{
		hostAddress = defConnection->localAddress();
	}

	//QMessageBox::information(NULL, "debug", hostAddress);

	userJid = AXmppStream->streamJid();
	//sipUri = userJid.pNode() + "@sip." + userJid.pDomain();
	sipUri = userJid.pNode() + "@" + userJid.pDomain();
	username = userJid.pNode();
	pass = AXmppStream->password();

	//QString str = "User: " + username + " Pass: " + pass;
	//QMessageBox::information(NULL, str, str);

	////////////if(username == "rvoip-1")
	////////////{
	////////////	sipUri = "\"ramtest1\" <sip:ramtest1@talkpad.ru>";
	////////////	username = "ramtest1";
	////////////	pass = "ramtest1";
	////////////}
	////////////else if(username == "spendtime" || username == "rvoip-2")
	////////////{
	////////////	sipUri = "\"ramtest2\" <sip:ramtest2@talkpad.ru>";
	////////////	username = "ramtest2";
	////////////	pass = "ramtest2";
	////////////}

	////////////QString res;
	////////////res += "username: " + username + " pass: " + pass + " sipUri: " + sipUri;

	////////////QMessageBox::information(NULL, "debug", res);

	//hostAddress = "192.168.255.72";
	//sipUri = "test_0@jabbertmp12.rambler.ru";
	//username = "test_0";
	//pass = "password";


	FSipPhoneProxy = new SipPhoneProxy(hostAddress, sipUri, username, pass, this);

	if(FSipPhoneProxy)
	{
		FSipPhoneProxy->initRegistrationData();
		connect(this, SIGNAL(sipSendInvite(const QString &)), FSipPhoneProxy, SLOT(makeNewCall(const QString&)));
		//connect(FSipPhoneProxy, SIGNAL(), this, SLOT());
		connect(this, SIGNAL(sipSendUnRegister()), FSipPhoneProxy, SLOT(makeClearRegisterProxySlot()));
		connect(FSipPhoneProxy, SIGNAL(callDeletedProxy(bool)), this, SLOT(sipCallDeletedSlot(bool)));
		connect(FSipPhoneProxy, SIGNAL(incomingThreadTimeChange(qint64)), this, SLOT(incomingThreadTimeChanged(qint64)));
	}

	connect(this, SIGNAL(streamRemoved(const QString&)), this, SLOT(sipClearRegistration(const QString&)));
	//connect(this, SIGNAL(streamRemoved(const QString&)), this, SLOT(tabControlRemove(const QString&)));

	connect(this, SIGNAL(streamCreated(const QString&)), this, SLOT(onStreamCreated(const QString&)));
	//
	//
}

void SipPhone::incomingThreadTimeChanged(qint64 timeMS)
{
	QList<RCallControl*> controls = FCallControls.values();

	QTime time(0,0,0);
	QTime time1 = time.addMSecs(timeMS);
	//time.addMSecs(timeMS);
	QString timeString = time1.toString("hh:mm:ss");

	foreach(RCallControl* control, controls)
	{
		if(control->status() == RCallControl::Accepted)
		{

			control->statusTextChange(timeString);
		}

	}

}

void SipPhone::onMetaTabWindowCreated(IMetaTabWindow* iMetaTabWindow)
{
	ToolBarChanger * tbChanger = iMetaTabWindow->toolBarChanger();
	// Далее добавляем кнопку звонка в tbChanger
	if(iMetaTabWindow->isContactPage() && tbChanger != NULL)
	{
		Action* callAction = new Action(tbChanger);
		callAction->setText(tr("Call"));
		//callAction->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_ARROW_LEFT);

		Jid streamJid;
		Jid contactJid;
		QString metaid = iMetaTabWindow->metaId();

		IMetaRoster* iMetaRoster = iMetaTabWindow->metaRoster();
		if(iMetaRoster !=NULL)
		{
			streamJid = iMetaRoster->streamJid();
			IMetaContact iMetaContact = iMetaRoster->metaContact(metaid);
			if(iMetaContact.items.size() > 0)
				contactJid = iMetaContact.items.values().at(0); // ???????
		}
		callAction->setData(ADR_STREAM_JID, streamJid.full());
		callAction->setData(ADR_CONTACT_JID, contactJid.full());
		callAction->setData(ADR_METAID_WINDOW, metaid);
		callAction->setCheckable(true);

		callAction->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON);

		connect(callAction, SIGNAL(triggered(bool)), SLOT(onToolBarActionTriggered(bool)));
		QLabel * separator = new QLabel;
		separator->setFixedWidth(12);
		separator->setPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_SEPARATOR)));
		tbChanger->insertWidget(separator, TBG_MCMTW_P2P_CALL);
		QToolButton * btn = tbChanger->insertAction(callAction, TBG_MCMTW_P2P_CALL);
		btn->setObjectName("tbSipCall");

		// Сохраняем указатель на кнопку. Понадобится для работы с ней. (изменение состояния при открытии/закрытии панели звонков программно)
		FCallActions.insert(metaid, callAction);
	}
}

void SipPhone::onToolBarActionTriggered(bool status)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		QString metaId = action->data(ADR_METAID_WINDOW).toString();

		if(status)
		{
			if(isSupported(streamJid, metaId))// contactJid))
			{
				Jid contactJidFull = getContactWithPresence(streamJid, metaId);
				if(contactJidFull.isValid() && !contactJidFull.isEmpty() && FStreams.isEmpty())
				{

					if(!FCallControls.contains(metaId))
					{
						IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(streamJid, metaId);
						if(iMetaTabWindow != NULL)
						{
							RCallControl* pCallControl = new RCallControl(RCallControl::Caller, iMetaTabWindow->instance()); /*status = Register*/
							pCallControl->setStreamJid(streamJid);
							pCallControl->setMetaId(metaId);
							//pCallControl->callStatusChange(RCallControl::Ringing);
							connect(pCallControl, SIGNAL(redialCall()), SLOT(onRedialCall()));
							//connect(pCallControl, SIGNAL(hangupCall()), SLOT(onHangupCallTest())); /*Test*/
							connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
							connect(pCallControl, SIGNAL(abortCall()), this, SLOT(onAbortCall()));
							connect(pCallControl, SIGNAL(closeAndDelete(bool)), action, SLOT(setChecked(bool)));

							connect(pCallControl, SIGNAL(startCamera()), FSipPhoneProxy, SIGNAL(proxyStartCamera()));
							connect(pCallControl, SIGNAL(stopCamera()), FSipPhoneProxy, SIGNAL(proxyStopCamera()));
							connect(pCallControl, SIGNAL(micStateChange(bool)), FSipPhoneProxy, SIGNAL(proxySuspendStateChange(bool)));

							iMetaTabWindow->insertTopWidget(0, pCallControl);
							FCallControls.insert(metaId, pCallControl);
						}
					}
					else
					{
						RCallControl* pCallControl = FCallControls[metaId];
						if(pCallControl)
						{
							pCallControl->callStatusChange(RCallControl::Register);
						}
					}

					//QMessageBox::information(NULL, contactJidFull.full(), "Call");
					QString sid = openStream(streamJid, contactJidFull);

					//////////////if(!FCallControls.contains(metaId))
					//////////////{
					//////////////	IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(streamJid, metaId);
					//////////////	if(iMetaTabWindow != NULL)
					//////////////	{
					//////////////		//Jid cItem = iMetaTabWindow->currentItem();
					//////////////		RCallControl* pCallControl = new RCallControl(sid, RCallControl::Caller, iMetaTabWindow->instance());
					//////////////		pCallControl->setStreamJid(streamJid);
					//////////////		pCallControl->setMetaId(metaId);
					//////////////		//pCallControl->callStatusChange(RCallControl::Ringing);
					//////////////		//connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
					//////////////		connect(pCallControl, SIGNAL(redialCall()), SLOT(onRedialCall()));
					//////////////		//connect(pCallControl, SIGNAL(hangupCall()), SLOT(onHangupCallTest()));
					//////////////		connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
					//////////////		connect(pCallControl, SIGNAL(abortCall()), this, SLOT(onAbortCall()));
					//////////////		connect(pCallControl, SIGNAL(closeAndDelete(bool)), action, SLOT(setChecked(bool)));

					//////////////		connect(pCallControl, SIGNAL(startCamera()), FSipPhoneProxy, SIGNAL(proxyStartCamera()));
					//////////////		connect(pCallControl, SIGNAL(stopCamera()), FSipPhoneProxy, SIGNAL(proxyStopCamera()));
					//////////////		connect(pCallControl, SIGNAL(micStateChange(bool)), FSipPhoneProxy, SIGNAL(proxySuspendStateChange(bool)));
					//////////////		//connect(pCallControl, SIGNAL(micStateChange(bool)), SLOT(onProxySuspendStateChange(bool)));

					//////////////		iMetaTabWindow->insertTopWidget(0, pCallControl);
					//////////////		FCallControls.insert(metaId, pCallControl);
					//////////////	}
					//////////////}
					//////////////else
					//////////////{
					//////////////	RCallControl* pCallControl = FCallControls[metaId];
					//////////////	if(pCallControl)
					//////////////	{
					//////////////		pCallControl->callStatusChange(RCallControl::Ringing);
					//////////////	}
					//////////////}

				}
			}
			else
			{
				QMessageBox::information(NULL, tr("Call failure"), tr("Calls NOT supported for current contact"));
				action->setChecked(false);
			}
		}
		else // status == false
		{
			if(FCallControls.contains(metaId))
			{
				RCallControl* pCallControl = FCallControls[metaId];

				IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(pCallControl->getStreamJid(), pCallControl->getMetaId());
				if(iMetaTabWindow != NULL)
				{
					iMetaTabWindow->removeTopWidget(pCallControl);
				}

				if(FCallControls.contains(metaId))
				{
					FCallControls.remove(metaId);
					pCallControl->close();
					delete pCallControl;
					pCallControl = NULL;
				}
			}
		}

	}
}

void SipPhone::onStreamClosed(IXmppStream *)
{
	if(FSipPhoneProxy != NULL)
	{
		delete FSipPhoneProxy;
		FSipPhoneProxy = NULL;
	}
}



bool SipPhone::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISipRequest == AHandleId)
	{
		QDomElement actionElem = AStanza.firstElement("query",NS_RAMBLER_SIP_PHONE).firstChildElement();
		QString sid = actionElem.attribute("sid");
		if (actionElem.tagName() == "open")
		{
			AAccept = true;
			// Здесь проверяем возможность установки соединения
			if (FStreams.contains(sid))
			{
				Stanza error = AStanza.replyError(ErrorHandler::coditionByCode(ErrorHandler::CONFLICT));
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else if (!findStream(AStreamJid,AStanza.from()).isEmpty())
			{
				Stanza error = AStanza.replyError(ErrorHandler::coditionByCode(ErrorHandler::NOT_ACCEPTABLE));
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else
			{
				//Здесь все проверки пройдены, заводим сессию и уведомляем пользователя о входящем звонке
				ISipStream stream;
				stream.sid = sid;
				stream.streamJid = AStreamJid;
				stream.contactJid = AStanza.from();
				stream.kind = ISipStream::SK_RESPONDER;
				stream.state = ISipStream::SS_OPEN;
				FStreams.insert(sid,stream);
				FPendingRequests.insert(sid,AStanza.id());
				insertNotify(stream);
				// И окно чата отображаем и панель управления
				showCallControlTab(sid);
				emit streamCreated(sid);
			}
		}
		else if (actionElem.tagName() == "close")
		{
			AAccept = true;
			FPendingRequests.insert(sid,AStanza.id());
			closeStream(sid);
		}
	}
	return false;
}

void SipPhone::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FOpenRequests.contains(AStanza.id()))
	{
		QString sid = FOpenRequests.take(AStanza.id());
		QDomElement actionElem = AStanza.firstElement("query",NS_RAMBLER_SIP_PHONE).firstChildElement();
		if (AStanza.type() == "result")
		{
			if (actionElem.tagName()=="opened" && actionElem.attribute("sid")==sid)
			{
				// Удаленный пользователь принял звонок, устанавливаем соединение
				// Для протокола SIP это означает следующие действия в этом месте:
				// -1) Регистрация на сарвере SIP уже должна быть выполнена!
				// 1) Отправка запроса INVITE
				//connect(this, SIGNAL(sipSendInvite(const QString&)),
				//				this, SLOT(sipSendInviteSlot(const QString&)));
				//emit sipSendInvite((username == "ramtest1") ? "ramtest2@talkpad.ru" : "ramtest1@talkpad.ru");
				QString uri = AStanza.from();
				int indexSlash = uri.indexOf("/");
				uri = uri.left(indexSlash);
				//QMessageBox::information(NULL, "", uri);

				// !!!!!!! ВНИМАНИЕ ВКЛЮЧИТЬ !!!!!!!
				emit sipSendInvite(uri);
				// 2) Получение акцепта на запрос INVITE
				// 3) Установка соединения
				ISipStream& stream = FStreams[sid];
				stream.state = ISipStream::SS_OPENED;
				emit streamStateChanged(sid, stream.state);
			}
			else // Пользователь отказался принимать звонок
			{

				removeStream(sid);
				// Здесь нужно выполнить отмену регистрации SIP
				//emit sipSendUnRegister();
			}
		}
		else
		{
			// Получили ошибку, по её коду можно определить причину, уведомляем пользоователя в окне звонка и закрываем сессию
			removeStream(sid);
			// Здесь нужно выполнить отмену регистрации SIP
			//emit sipSendUnRegister();
		}
	}
	else if (FCloseRequests.contains(AStanza.id()))
	{
		// Получили ответ на закрытие сессии, есть ошибка или нет уже не важно
		QString sid = FCloseRequests.take(AStanza.id());
		removeStream(sid);
		// Здесь нужно выполнить отмену регистрации SIP
		//emit sipSendUnRegister();
	}
}


//void SipPhone::sipSendInviteSlot(const QString &AClientSIP)
//{
//	sipActionAfterInviteAnswer(true, AClientSIP);
//}
//
void SipPhone::sipActionAfterInviteAnswer(bool AInviteStatus, const QString &AClientSIP)
{
	if(AInviteStatus == true)
	{

	}
	else
	{
		// Получили отказ. Закрываем соединение.
	}
}

void SipPhone::onTabActionHangup()
{
	//RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());

	//if(pCallControl->side() == RCallControl::Caller)
	//{

	//}
	//if (pCallControl && pCallControl->parentWidget())
	//{
	//	IMetaTabWindow* iMetaTab = qobject_cast<IMetaTabWindow *>(pCallControl->parentWidget());
	//	if(iMetaTab)
	//	{
	//		FCallControls[iMetaTab->metaId().full();]
	//
	//	}
	//}
}

void SipPhone::sipCallDeletedSlot(bool initiator)
{
	emit sipSendUnRegister();
	if(initiator)
	{
		QString str = "closeStream(" + streamId + ")";
		//QMessageBox::information(NULL, "debug", str);
		closeStream(streamId);
		//removeStream(streamId);
	}
}
void SipPhone::sipClearRegistration(const QString&)
{
	//emit sipSendUnRegister();
}



void SipPhone::onStreamCreated(const QString& sid)
{
	//QMessageBox::information(NULL, "debug", sid);
	streamId = sid;

	if(FStreams.contains(sid))
	{
		ISipStream stream = FStreams.value(sid);
		QString metaId = findMetaId(stream.streamJid, stream.contactJid);

		if(metaId.isEmpty())
			return;

		// Если панель звонка еще не отображена, то отображаем
		if(!FCallControls.contains(metaId))
		{
			IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
			if(iMetaTabWindow != NULL)
			{
				RCallControl* pCallControl = new RCallControl(sid, RCallControl::Caller, iMetaTabWindow->instance());
				pCallControl->setStreamJid(stream.streamJid);
				pCallControl->setMetaId(metaId);
				//pCallControl->callStatusChange(RCallControl::Ringing);
				//connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
				connect(pCallControl, SIGNAL(redialCall()), SLOT(onRedialCall()));
				//connect(pCallControl, SIGNAL(hangupCall()), SLOT(onHangupCallTest()));
				connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
				connect(pCallControl, SIGNAL(abortCall()), this, SLOT(onAbortCall()));

				if(FCallActions.contains(metaId) && FCallActions[metaId])
					connect(pCallControl, SIGNAL(closeAndDelete(bool)), FCallActions[metaId], SLOT(setChecked(bool)));
				//connect(pCallControl, SIGNAL(closeAndDelete(bool)), action, SLOT(setChecked(bool)));

				connect(pCallControl, SIGNAL(startCamera()), FSipPhoneProxy, SIGNAL(proxyStartCamera()));
				connect(pCallControl, SIGNAL(stopCamera()), FSipPhoneProxy, SIGNAL(proxyStopCamera()));
				connect(pCallControl, SIGNAL(micStateChange(bool)), FSipPhoneProxy, SIGNAL(proxySuspendStateChange(bool)));
				//connect(pCallControl, SIGNAL(micStateChange(bool)), SLOT(onProxySuspendStateChange(bool)));

				iMetaTabWindow->insertTopWidget(0, pCallControl);
				FCallControls.insert(metaId, pCallControl);
			}
		}
		else // Панель звонка отображена - обновляем статус
		{
			RCallControl* pCallControl = FCallControls[metaId];
			if(pCallControl)
			{
				pCallControl->setSessionId(sid);
				pCallControl->callStatusChange(RCallControl::Ringing);
			}
		}
	}





	//if(FStreams.contains(sid))
	//{
	//	ISipStream stream = FStreams.value(sid);
	//	QString metaId = findMetaId(stream.streamJid, stream.contactJid);

	//	if(metaId.isEmpty())
	//		return;

	//	if(FCallControls.contains(metaId))
	//	{
	//		RCallControl* pCallControl = FCallControls[metaId];
	//		if(pCallControl->side() == RCallControl::Caller)
	//		{
	//			pCallControl->setSessionId(sid);
	//			pCallControl->callStatusChange(RCallControl::Ringing);
	//		}
	//	}
	//}

}

void SipPhone::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FOpenRequests.contains(AStanzaId))
	{
		// Удаленная сторона не ответила на звонок, закрываем соединение
		QString sid = FOpenRequests.take(AStanzaId);
		// Если нет ответа от принимающей стороны, то устанавливаем соответствующий флаг
		ISipStream& stream = FStreams[sid];
		stream.noAnswer = true;
		closeStream(sid);
	}
	else if (FCloseRequests.contains(AStanzaId))
	{
		// Нет ответа на закрытие соединения, считаем сесиию закрытой
		QString sid = FCloseRequests.take(AStanzaId);
		removeStream(sid);
	}
}

bool SipPhone::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	//if(FDiscovery == NULL)
	//{
	//	qDebug(" SipPhone::supported 1");
	//	return true;
	//}
	//
	//IDiscoInfo dInfo = FDiscovery->discoInfo(AStreamJid, AContactJid);
	//QString strInfo = "SipPhone: StreamJid: " + AStreamJid.pFull() + " ContactJid: " + AContactJid.pFull();
	//qDebug(strInfo.toAscii());

	//QStringList fList = dInfo.features;
	//foreach(QString str, fList)
	//{
	//	qDebug(str.toAscii());
	//}
	//if(fList.contains(NS_RAMBLER_SIP_PHONE))
	//{
	//	qDebug(" SipPhone::supported 2");
	//	return true;
	//}

	//qDebug(" SipPhone::NOT supported");
	//return false;

	return FDiscovery == NULL || FDiscovery->discoInfo(AStreamJid, AContactJid).features.contains(NS_RAMBLER_SIP_PHONE);
}

QList<QString> SipPhone::streams() const
{
	return FStreams.keys();
}

ISipStream SipPhone::streamById(const QString &AStreamId) const
{
	return FStreams.value(AStreamId);
}

QString SipPhone::findStream(const Jid &AStreamJid, const Jid &AContactJid) const
{
	for (QMap<QString, ISipStream>::const_iterator it=FStreams.constBegin(); it!=FStreams.constEnd(); it++)
	{
		if (it->streamJid==AStreamJid && it->contactJid==AContactJid)
			return it->sid;
	}
	return QString::null;
}

// Отмена звонка пользователем инициатором
void SipPhone::onAbortCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if (pCallControl)
	{
		QString streamId = pCallControl->getSessionID();
		if(streamId != "")
			closeStream(streamId);
		else
		{
			IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(pCallControl->getStreamJid(), pCallControl->getMetaId());
			if(iMetaTabWindow != NULL)
			{
				iMetaTabWindow->removeTopWidget(pCallControl);
			}

			emit sipSendUnRegister();

			if(FCallControls.contains(pCallControl->getMetaId()))
			{
				FCallControls.remove(pCallControl->getMetaId());
				if(pCallControl != NULL)
				{
					delete pCallControl;
					pCallControl = NULL;
				}
			}


			//if(FCallControls.contains(pCallControl->getMetaId()))
			//{
			//	RCallControl* pCallControl = FCallControls[metaId];
			//	IMetaTabWindow* iMetaTabWindow = qobject_cast<IMetaTabWindow*>(pCallControl->parentWidget());
			//	if(iMetaTabWindow)
			//		iMetaTabWindow->removeTopWidget(pCallControl);
			//	FCallControls.remove(metaId);
			//	delete pCallControl;
			//	pCallControl = NULL;
			//}
		}
	}

#pragma message("SipPhone::onAbortCall() not implemented")
}

void SipPhone::onRedialCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if(pCallControl)
	{
		Jid contactJidFull = getContactWithPresence(pCallControl->getStreamJid(), pCallControl->getMetaId());
		if(contactJidFull.isValid() && !contactJidFull.isEmpty())
		{
			pCallControl->callStatusChange(RCallControl::Ringing);
			QString sid = openStream(pCallControl->getStreamJid(), contactJidFull);
			//pCallControl->setSessionId(sid);
		}
	}
}

void SipPhone::onHangupCallTest()
{
	sipCallDeletedSlot(true);
}

void SipPhone::onStreamStateChanged(const QString& sid, int state)
{
	QString dString = "SipPhone::onStreamStateChanged " + sid + " state: " + QString::number(state);
	qDebug(dString.toAscii());

	if(!FStreams.contains(sid))
		return;

	ISipStream stream = FStreams.value(sid);
	QString metaId = findMetaId(stream.streamJid, stream.contactJid);

	if(metaId.isEmpty())
		return;

	if(!FCallControls.contains(metaId))
		return;

	RCallControl* pCallControl = FCallControls.value(metaId);
	if(pCallControl == NULL)
		return;

	if(pCallControl->side() == RCallControl::Caller)
	{
		if(state == ISipStream::SS_OPENED)
		{
			pCallControl->callStatusChange(RCallControl::Accepted);
		}
		else if(state == ISipStream::SS_CLOSE) // Хотим повесить трубку
		{
			// Если нет ответа за таймаут от принимающей стороны, то не закрываем панель,
			// устанавливаем соответствующий статус для возможности совершения повторного вызова
			if(stream.noAnswer)
			{
				pCallControl->callStatusChange(RCallControl::RingTimeout);
			}
			else
			{
				IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
				if(iMetaTabWindow != NULL)
				{
					iMetaTabWindow->removeTopWidget(pCallControl);
				}
				FCallControls.remove(metaId);
				delete pCallControl;
				pCallControl = NULL;
			}
		}
		else if(state == ISipStream::SS_CLOSED) // Удаленный пользователь повесил трубку
		{
			if(pCallControl->status() == RCallControl::Ringing)
			{
				// Говорим что пользователь не захотел брать трубку. Дальнейшие действия:
				pCallControl->callStatusChange(RCallControl::Hangup);
			}
			else if(pCallControl->status() == RCallControl::Accepted) // Удаленный пользователь повесил трубку во время разговора. Нам тоже надо.
			{
				IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
				if(iMetaTabWindow != NULL)
				{
					iMetaTabWindow->removeTopWidget(pCallControl);
				}
				FCallControls.remove(metaId);
				delete pCallControl;
				pCallControl = NULL;
			}
		}
	}
	else if(pCallControl->side() == RCallControl::Receiver)
	{
		if(state == ISipStream::SS_OPENED)
		{
			pCallControl->callStatusChange(RCallControl::Accepted);
		}
		else if(state == ISipStream::SS_CLOSE || state == ISipStream::SS_CLOSED)
		{
			IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
			if(iMetaTabWindow != NULL)
			{
				iMetaTabWindow->removeTopWidget(pCallControl);
			}
			FCallControls.remove(metaId);
			delete pCallControl;
			pCallControl = NULL;
		}
		else if(state == ISipStream::SS_CLOSED)
		{

		}

	}

}

void SipPhone::onAcceptStreamByCallControl()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if(pCallControl)
	{
		QString sid = pCallControl->getSessionID();
		acceptStream(sid);
	}
}

QString SipPhone::openStream(const Jid &AStreamJid, const Jid &AContactJid)
{
	// Тестовый вариант установки соединения
	//////////////Stanza open("iq");
	//////////////open.setType("set").setId(FStanzaProcessor->newId()).setTo(AContactJid.eFull());
	//////////////QDomElement openElem = open.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(open.createElement("open")).toElement();
	//////////////
	//////////////QString sid = QUuid::createUuid().toString();
	//////////////openElem.setAttribute("sid",sid);
	//////////////// Здесь добавляем нужные параметры для установки соединения в элемент open
	//////////////
	//////////////if (FStanzaProcessor->sendStanzaRequest(this, AStreamJid, open, REQUEST_TIMEOUT))
	//////////////{
	//////////////	ISipStream stream;
	//////////////	stream.sid = sid;
	//////////////	stream.streamJid = AStreamJid;
	//////////////	stream.contactJid = AContactJid;
	//////////////	stream.kind = ISipStream::SK_INITIATOR;
	//////////////	stream.state = ISipStream::SS_OPEN;
	//////////////	FStreams.insert(sid,stream);
	//////////////	FOpenRequests.insert(open.id(),sid);
	//////////////	emit streamCreated(sid);
	//////////////	return sid;
	//////////////}


	// ПОДКЛЮЧЕНИЕ SIP
	//if (FStanzaProcessor)// && isSupported(AStreamJid,AContactJid))
	if (FStanzaProcessor && isSupported(AStreamJid, AContactJid))
	{
		connect(this, SIGNAL(sipSendRegisterAsInitiator(const Jid&,const Jid&)),
			FSipPhoneProxy, SLOT(makeRegisterProxySlot(const Jid&, const Jid&)));

		connect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const Jid&, const Jid&)),
			this, SLOT(sipActionAfterRegistrationAsInitiator(bool, const Jid&, const Jid&)));

		emit sipSendRegisterAsInitiator(AStreamJid, AContactJid);

		//Stanza open("iq");
		//open.setType("set").setId(FStanzaProcessor->newId()).setTo(AContactJid.eFull());
		//QDomElement openElem = open.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(open.createElement("open")).toElement();
		//
		//QString sid = QUuid::createUuid().toString();
		//openElem.setAttribute("sid",sid);
		//// Здесь добавляем нужные параметры для установки соединения в элемент open
		//
		//if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,open,REQUEST_TIMEOUT))
		//{
		//	ISipStream stream;
		//	stream.sid = sid;
		//	stream.streamJid = AStreamJid;
		//	stream.contactJid = AContactJid;
		//	stream.kind = ISipStream::SK_INITIATOR;
		//	stream.state = ISipStream::SS_OPEN;
		//	FStreams.insert(sid,stream);
		//	FOpenRequests.insert(open.id(),sid);
		//	emit streamCreated(sid);
		//	return sid;
		//}
	}
	return QString::null;
}

void SipPhone::sipActionAfterRegistrationAsInitiator(bool ARegistrationResult, const Jid& AStreamJid, const Jid& AContactJid)
{
	disconnect(this, SIGNAL(sipSendRegisterAsInitiator(const Jid&,const Jid&)), 0, 0);
	disconnect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const Jid&, const Jid&)), 0, 0);


	if(ARegistrationResult)
	{
		//QMessageBox::information(NULL, "debug", "sipActionAfterRegistrationAsInitiator:: true");
		Stanza open("iq");
		open.setType("set").setId(FStanzaProcessor->newId()).setTo(AContactJid.eFull());
		QDomElement openElem = open.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(open.createElement("open")).toElement();

		QString sid = QUuid::createUuid().toString();
		openElem.setAttribute("sid",sid);
		// Здесь добавляем нужные параметры для установки соединения в элемент open


		// Переводим панель в режим Ringing
		QString metaId = findMetaId(AStreamJid, AContactJid);
		if(FCallControls.contains(metaId))
		{
			FCallControls[metaId]->setSessionId(sid);
			FCallControls[metaId]->callStatusChange(RCallControl::Ringing );
		}




		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,open,REQUEST_TIMEOUT))
		{
			ISipStream stream;
			stream.sid = sid;
			stream.streamJid = AStreamJid;
			stream.contactJid = AContactJid;
			stream.kind = ISipStream::SK_INITIATOR;
			stream.state = ISipStream::SS_OPEN;
			FStreams.insert(sid,stream);
			FOpenRequests.insert(open.id(),sid);
			emit streamCreated(sid);
			//return sid;
		}
	}
	else
	{
		// НОТИФИКАЦИЯ О НЕУДАЧНОЙ ?ЕГИСТ?АЦИИ
		//QMessageBox::information(NULL, "debug", "sipActionAfterRegistrationAsInitiator:: false");
		//QMessageBox::information(NULL, "SIP Reistration failed", "SIP registration failed.");

		// Скрываем панель звонка
		QString metaId = findMetaId(AStreamJid, AContactJid);
		if(FCallControls.contains(metaId))
		{
			RCallControl* pCallControl = FCallControls[metaId];
			IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(pCallControl->getStreamJid(), pCallControl->getMetaId());
			if(iMetaTabWindow != NULL)
			{
				iMetaTabWindow->removeTopWidget(pCallControl);
			}

			FCallControls.remove(metaId);
			pCallControl->callStatusChange(RCallControl::Undefined);
			delete pCallControl;
			pCallControl = NULL;
		}
	}
}

//void SipPhone::sipRegisterInitiatorSlot(const Jid& AStreamJid, const Jid& AContactJid)
//{
//	sipActionAfterRegistrationAsInitiator(true, AStreamJid, AContactJid);
//}



// Responder part
bool SipPhone::acceptStream(const QString &AStreamId)
{
	// Тестовый вариант установки соединения
	//////////////ISipStream &stream = FStreams[AStreamId];

	//////////////Stanza opened("iq");
	//////////////opened.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
	//////////////QDomElement openedElem = opened.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(opened.createElement("opened")).toElement();
	//////////////openedElem.setAttribute("sid",AStreamId);

	//////////////if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
	//////////////{
	//////////////	FPendingRequests.remove(AStreamId);
	//////////////	stream.state = ISipStream::SS_OPENED;
	//////////////	removeNotify(AStreamId);
	//////////////	emit streamStateChanged(AStreamId, stream.state);
	//////////////	return true;
	//////////////}

	// ПОДКЛЮЧЕНИЕ SIP
	if (FStanzaProcessor && FPendingRequests.contains(AStreamId))
	{
		connect(this, SIGNAL(sipSendRegisterAsResponder(const QString&)),
			FSipPhoneProxy, SLOT(makeRegisterResponderProxySlot(const QString&)));

		connect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const QString&)),
			this, SLOT(sipActionAfterRegistrationAsResponder(bool, const QString&)));

		// Переводим панель в режим Register
		ISipStream stream = FStreams[AStreamId];
		QString metaId = findMetaId(stream.streamJid, stream.contactJid);
		if(FCallControls.contains(metaId))
		{
			RCallControl* pCallControl = FCallControls[metaId];
			pCallControl->callStatusChange(RCallControl::Register);
		}

		// Сигнализируем о необходимости SIP регистрации клиента
		emit sipSendRegisterAsResponder(AStreamId);



		//ISipStream &stream = FStreams[AStreamId];

		//Stanza opened("iq");
		//opened.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
		//QDomElement openedElem = opened.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(opened.createElement("opened")).toElement();
		//openedElem.setAttribute("sid",AStreamId);

		//if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
		//{
		//	FPendingRequests.remove(AStreamId);
		//	stream.state = ISipStream::SS_OPENED;
		//	removeNotify(AStreamId);
		//	emit streamStateChanged(AStreamId, stream.state);
		//	return true;
		//}
	}
	return false;
}

void SipPhone::sipActionAfterRegistrationAsResponder(bool ARegistrationResult, const QString &AStreamId)
{
	disconnect(this, SIGNAL(sipSendRegisterAsResponder(const QString&)), 0, 0);
	disconnect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const QString&)), 0, 0);

	if(ARegistrationResult)
	{
		//QMessageBox::information(NULL, "", "sipActionAfterRegistrationAsResponder");
		ISipStream &stream = FStreams[AStreamId];

		Stanza opened("iq");
		opened.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
		QDomElement openedElem = opened.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(opened.createElement("opened")).toElement();
		openedElem.setAttribute("sid",AStreamId);


		//////////////////////////// Переводим панель в режим Accepted
		//////////////////////////ISipStream stream = FStreams[AStreamId];
		//////////////////////////QString metaId = findMetaId(stream.streamJid, stream.contactJid);
		//////////////////////////if(FCallControls.contains(metaId))
		//////////////////////////{
		//////////////////////////	RCallControl* pCallControl = FCallControls[metaId];
		//////////////////////////	pCallControl->callStatusChange(RCallControl::Accepted );
		//////////////////////////}


		//QMessageBox::information(NULL, "", "sipActionAfterRegistrationAsResponder->sendStanzaOut");
		if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
		{
			FPendingRequests.remove(AStreamId);
			stream.state = ISipStream::SS_OPENED;
			removeNotify(AStreamId);
			emit streamStateChanged(AStreamId, stream.state);
			//return true;
		}
	}
	else
	{
		// Не удалось выполнить SIP регистрацию. Закрываем соединение.
		closeStream(AStreamId);
	}
}
//
//void SipPhone::sipRegisterResponderSlot(const QString& AStreamId)
//{
//	sipActionAfterRegistrationAsResponder(true, AStreamId );
//}

void SipPhone::finalActionAfterHangup()
{

}

void SipPhone::closeStream(const QString &AStreamId)
{
	if (FStanzaProcessor && FStreams.contains(AStreamId))
	{
		ISipStream &stream = FStreams[AStreamId];
		if (stream.state != ISipStream::SS_CLOSE)
		{
			bool isResult = FPendingRequests.contains(AStreamId);

			Stanza close("iq");
			QDomElement closeElem;
			if (isResult)
			{
				close.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
				closeElem = close.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(close.createElement("closed")).toElement();
			}
			else
			{
				close.setType("set").setId(FStanzaProcessor->newId()).setTo(stream.contactJid.eFull());
				closeElem = close.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(close.createElement("close")).toElement();
			}
			closeElem.setAttribute("sid",stream.sid);
			if (isResult ? FStanzaProcessor->sendStanzaOut(stream.streamJid,close) : FStanzaProcessor->sendStanzaRequest(this,stream.streamJid,close,CLOSE_TIMEOUT))
			{
				if (!isResult)
				{
					FCloseRequests.insert(close.id(),AStreamId);
					stream.state = ISipStream::SS_CLOSE;
					emit streamStateChanged(AStreamId,stream.state);
				}
				else
				{
					//QString str = "Remove " + AStreamId;
					//QMessageBox::information(NULL, "debug", str);
					removeStream(AStreamId);
				}
			}
			else
			{
				//Не удалось отправить запрос, возможно связь с сервером прервалась, считаем сессию закрытой
				removeStream(AStreamId);
			}
			FPendingRequests.remove(AStreamId);
			removeNotify(AStreamId);
		}
	}
}


QString SipPhone::findMetaId(const Jid& AStreamJid, const Jid& AContactJid) const
{
	IMetaRoster* iMetaRoster = FMetaContacts->findMetaRoster(AStreamJid);
	if(iMetaRoster == NULL || !iMetaRoster->isOpen())
		return QString();
	return iMetaRoster->itemMetaContact(AContactJid);
}


void SipPhone::showCallControlTab(const QString& sid/*const ISipStream &AStream*/)
{
	if(!FStreams.contains(sid))
		return;

	ISipStream& stream = FStreams[sid];

	if(!FMessageProcessor->createWindow(stream.streamJid, stream.contactJid, Message::Chat,IMessageHandler::SM_SHOW))
		return;

	//IMetaRoster* iMetaRoster = FMetaContacts->findMetaRoster(stream.streamJid);
	//if(iMetaRoster == NULL || !iMetaRoster->isOpen())
	//	return;
	//QString metaId = iMetaRoster->itemMetaContact(stream.contactJid);
	QString metaId = findMetaId(stream.streamJid, stream.contactJid);

	if(!FCallControls.contains(metaId))
	{
		IMetaTabWindow* iMetaTabWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
		if(iMetaTabWindow != NULL)
		{
			RCallControl* pCallControl = new RCallControl(sid, RCallControl::Receiver, iMetaTabWindow->instance());
			pCallControl->callStatusChange(RCallControl::Ringing);
			//connect(pCallControl, SIGNAL(hangupCall()), SLOT(onHangupCallTest()));
			connect(pCallControl, SIGNAL(hangupCall()), FSipPhoneProxy, SLOT(hangupCall()));
			connect(pCallControl, SIGNAL(acceptCall()), this, SLOT(onAcceptStreamByCallControl()));
			connect(pCallControl, SIGNAL(abortCall()), this, SLOT(onAbortCall()));

			//connect(pCallControl, SIGNAL(closeAndDelete(bool)), action, SLOT(setChecked(bool)));
			if(FCallActions.contains(metaId) && FCallActions[metaId])
				connect(pCallControl, SIGNAL(closeAndDelete(bool)), FCallActions[metaId], SLOT(setChecked(bool)));

			// ?еакция на изменение состояния камеры
			connect(pCallControl, SIGNAL(startCamera()), FSipPhoneProxy, SIGNAL(proxyStartCamera()));
			connect(pCallControl, SIGNAL(stopCamera()), FSipPhoneProxy, SIGNAL(proxyStopCamera()));
			connect(pCallControl, SIGNAL(micStateChange(bool)), FSipPhoneProxy, SIGNAL(proxySuspendStateChange(bool)));


			iMetaTabWindow->insertTopWidget(0, pCallControl);
			FCallControls.insert(metaId, pCallControl);
		}
	}
	else
	{
		RCallControl* pCallControl = FCallControls[metaId];
		if(pCallControl)
		{
			if(pCallControl->side() == RCallControl::Caller)
			{
				pCallControl->callSideChange(RCallControl::Receiver);
			}
			pCallControl->setSessionId(sid);
			pCallControl->callStatusChange(RCallControl::Ringing);
		}
	}

	//Jid contactJidFull = getContactWithPresence(streamJid, metaId);
	//if(contactJidFull.isValid() && !contactJidFull.isEmpty())
	//{
	//	//QMessageBox::information(NULL, contactJidFull.full(), "Call");
	//	openStream(streamJid, contactJidFull);
	//}

}

void SipPhone::insertNotify(const ISipStream &AStream)
{
	INotification notify;
	notify.kinds = FNotifications!=NULL ? FNotifications->notificatorKinds(NID_CHAT_MESSAGE) : 0;
	if (notify.kinds > 0)
	{
		QString message = tr("Calling you...");
		QString name = FNotifications->contactName(AStream.streamJid,AStream.contactJid);

		notify.notificatior = NID_SIPPHONE_CALL;
		notify.data.insert(NDR_STREAM_JID,AStream.streamJid.full());
		notify.data.insert(NDR_CONTACT_JID,AStream.contactJid.full());
		notify.data.insert(NDR_ICON_KEY,MNI_SIPPHONE_CALL);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_ORDER,RNO_SIPPHONE_CALL);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
		notify.data.insert(NDR_ROSTER_HOOK_CLICK,false);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
		notify.data.insert(NDR_ROSTER_FOOTER,message);
		notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::green));
		notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(message));
		notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SIP_CALL);
		notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,0);
		notify.data.insert(NDR_TABPAGE_CREATE_TAB,true);
		notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
		notify.data.insert(NDR_TABPAGE_TOOLTIP,message);
		notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_SIPPHONE_TABBARITEM_CALL);
		/*		notify.data.insert(NDR_POPUP_NOTICE,message);
  notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AStream.contactJid));
  notify.data.insert(NDR_POPUP_TITLE,name);
  notify.data.insert(NDR_POPUP_STYLEKEY,STS_SIPPHONE_NOTIFYWIDGET_CALL);
  notify.data.insert(NDR_POPUP_TIMEOUT,0);
  notify.data.insert(NDR_SOUND_FILE,SDF_SIPPHONE_CALL);*/
		SipCallNotifyer * callNotifyer = new SipCallNotifyer(name, tr("Incoming call"), IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL), FNotifications->contactAvatar(AStream.contactJid));

		callNotifyer->setProperty("streamId", AStream.sid);
		connect(callNotifyer, SIGNAL(accepted()), SLOT(onAcceptStreamByAction()));
		connect(callNotifyer, SIGNAL(rejected()), SLOT(onCloseStreamByAction()));

		callNotifyer->appear();

		Action *acceptCall = new Action(this);
		acceptCall->setText(tr("Accept"));
		acceptCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(acceptCall,SIGNAL(triggered()),SLOT(onAcceptStreamByAction()));
		notify.actions.append(acceptCall);

		Action *declineCall = new Action(this);
		declineCall->setText(tr("Decline"));
		declineCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(declineCall,SIGNAL(triggered()),SLOT(onCloseStreamByAction()));
		notify.actions.append(declineCall);

		FNotifies.insert(FNotifications->appendNotification(notify), AStream.sid);
	}
}

void SipPhone::removeNotify(const QString &AStreamId)
{
	if (FNotifications)
		FNotifications->removeNotification(FNotifies.key(AStreamId));
}

void SipPhone::removeStream(const QString &AStreamId)
{
	if (FStreams.contains(AStreamId))
	{
		ISipStream &stream = FStreams[AStreamId];
		stream.state = ISipStream::SS_CLOSED;
		emit streamStateChanged(AStreamId, stream.state);
		emit streamRemoved(AStreamId);
		FStreams.remove(AStreamId);
	}
}

void SipPhone::onOpenStreamByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		openStream(streamJid, contactJid);
		return;
	}
}

void SipPhone::onAcceptStreamByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		acceptStream(streamId);
	}
	else
	{
		SipCallNotifyer * callNotifyer = qobject_cast<SipCallNotifyer*>(sender());
		if (callNotifyer)
		{
			QString streamId = callNotifyer->property("streamId").toString();
			acceptStream(streamId);
		}
	}
}

void SipPhone::onCloseStreamByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		closeStream(streamId);
	}
	else
	{
		SipCallNotifyer * callNotifyer = qobject_cast<SipCallNotifyer*>(sender());
		if (callNotifyer)
		{
			QString streamId = callNotifyer->property("streamId").toString();
			closeStream(streamId);
		}
	}
}

void SipPhone::onNotificationActivated(int ANotifyId)
{
	acceptStream(FNotifies.value(ANotifyId));
}

void SipPhone::onNotificationRemoved(int ANotifyId)
{
	FNotifies.remove(ANotifyId);
}

void SipPhone::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	// В случае обычных контактов
	if (AIndex->type()==RIT_CONTACT && ASelected.count() < 2)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		if (isSupported(streamJid,contactJid))
		{
			if (findStream(streamJid,contactJid).isEmpty())
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Call"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
				action->setData(ADR_STREAM_JID,streamJid.full());
				action->setData(ADR_CONTACT_JID,contactJid.full());
				connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
				AMenu->addAction(action,AG_RVCM_SIPPHONE_CALL,true);
			}
		}
		return;
	}

	// В случае метаконтактов
	if ( AIndex->type()==RIT_METACONTACT && ASelected.count() < 2)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		QString metaId = AIndex->data(RDR_META_ID).toString();

		IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(streamJid);
		IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(streamJid) : NULL;

		if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
		{
			IMetaContact metaContact = metaRoster->metaContact(metaId);
			if(metaContact.items.size() > 0)
			{
				foreach(Jid contactJid, metaContact.items)
				{
					QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

					if(pItems.size() > 0)
					{
						foreach(IPresenceItem pItem, pItems)
						{
							Jid contactJidWithPresence = pItem.itemJid;
							if(isSupported(streamJid, contactJidWithPresence))
							{
								if (findStream(streamJid, contactJidWithPresence).isEmpty())
								{
									Action *action = new Action(AMenu);
									action->setText(tr("Call"));
									action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
									action->setData(ADR_STREAM_JID,streamJid.full());
									action->setData(ADR_CONTACT_JID,contactJidWithPresence.full());
									action->setData(ADR_METAID_WINDOW, metaId);
									connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
									AMenu->addAction(action,AG_RVCM_SIPPHONE_CALL,true);
								}
							}
						}
					}
				}
			}
		}
	}
}



void SipPhone::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolTips);
	// В случае обычных контактов
	if (ALabelId==RLID_DISPLAY && AIndex->type()==RIT_CONTACT)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		if (isSupported(streamJid, contactJid))
		{
			if (findStream(streamJid, contactJid).isEmpty())
			{
				Action *action = new Action(AToolBarChanger->toolBar());
				action->setText(tr("Call"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
				action->setData(ADR_STREAM_JID,streamJid.full());
				action->setData(ADR_CONTACT_JID,contactJid.full());
				connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
				AToolBarChanger->insertAction(action);
			}
		}
		return;
	}

	// В случае метаконтактов
	if (ALabelId==RLID_DISPLAY && AIndex->type()==RIT_METACONTACT)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		QString metaId = AIndex->data(RDR_META_ID).toString();

		IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(streamJid);
		IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(streamJid) : NULL;

		if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
		{
			IMetaContact metaContact = metaRoster->metaContact(metaId);
			if(metaContact.items.size() > 0)
			{
				foreach(Jid contactJid, metaContact.items)
				{
					QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

					if(pItems.size() > 0)
					{
						foreach(IPresenceItem pItem, pItems)
						{
							Jid contactJidWithPresence = pItem.itemJid;
							if(isSupported(streamJid, contactJidWithPresence))
							{
								if (findStream(streamJid, contactJidWithPresence).isEmpty())
								{
									Action *action = new Action(AToolBarChanger->toolBar());
									action->setText(tr("Call"));
									action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
									action->setData(ADR_STREAM_JID,streamJid.full());
									action->setData(ADR_CONTACT_JID,contactJidWithPresence.full());
									//action->setData(ADR_CONTACT_JID,contactJid.full());
									action->setData(ADR_METAID_WINDOW, metaId);
									connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
									AToolBarChanger->insertAction(action);
								}
							}
						}
					}
				}
			}
		}
	}

}


bool SipPhone::isSupported(const Jid &AStreamJid, const QString &AMetaId) const
{
	IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(AStreamJid);
	IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(AStreamJid) : NULL;

	if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
	{
		IMetaContact metaContact = metaRoster->metaContact(AMetaId);
		if(metaContact.items.size() > 0)
		{
			foreach(Jid contactJid, metaContact.items)
			{
				QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

				if(pItems.size() > 0)
				{
					foreach(IPresenceItem pItem, pItems)
					{
						Jid contactJidWithPresence = pItem.itemJid;
						if(isSupported(AStreamJid, contactJidWithPresence))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool SipPhone::isSupportedAndFindStream(const Jid &AStreamJid, const QString &AMetaId, /*out*/QString& AStreamID) const
{
	IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(AStreamJid);
	IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(AStreamJid) : NULL;

	if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
	{
		IMetaContact metaContact = metaRoster->metaContact(AMetaId);
		if(metaContact.items.size() > 0)
		{
			foreach(Jid contactJid, metaContact.items)
			{
				QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

				if(pItems.size() > 0)
				{
					foreach(IPresenceItem pItem, pItems)
					{
						Jid contactJidWithPresence = pItem.itemJid;
						if(isSupported(AStreamJid, contactJidWithPresence))
						{
							AStreamID = findStream(AStreamJid, contactJidWithPresence);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

Jid SipPhone::getContactWithPresence(const Jid &AStreamJid, const QString &AMetaId) const
{
	IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(AStreamJid);
	IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(AStreamJid) : NULL;

	if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
	{
		IMetaContact metaContact = metaRoster->metaContact(AMetaId);
		if(metaContact.items.size() > 0)
		{
			foreach(Jid contactJid, metaContact.items)
			{
				QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

				if(pItems.size() > 0)
				{
					foreach(IPresenceItem pItem, pItems)
					{
						Jid contactJidWithPresence = pItem.itemJid;
						if(isSupported(AStreamJid, contactJidWithPresence))
						{
							return contactJidWithPresence;
						}
					}
				}
			}
		}
	}
	return Jid::null;
}





Q_EXPORT_PLUGIN2(plg_sipphone, SipPhone)
