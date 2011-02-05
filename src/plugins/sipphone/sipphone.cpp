#include "sipphone.h"
#include <QMessageBox>

#define SHC_SIP_REQUEST "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SIP_PHONE "']"

#define ADR_STREAM_JID  Action::DR_StreamJid
#define ADR_CONTACT_JID Action::DR_Parametr1
#define ADR_STREAM_ID   Action::DR_Parametr2

#define CLOSE_TIMEOUT   10000
#define REQUEST_TIMEOUT 30000

SipPhone::SipPhone()
{
	FDiscovery = NULL;
	FStanzaProcessor = NULL;
	FNotifications = NULL;
	FRostersViewPlugin = NULL;

	FSHISipRequest = -1;

	FSipPhoneProxy = NULL;
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
	APluginInfo->author = "Panov S";
	APluginInfo->homePage = "http://virtus.rambler.ru";
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

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
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
		sipPhone.description = tr("SIP voice ans video calls");
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


	//if(username == "rvoip-1")
	//{
	//	sipUri = "\"ramtest1\" <sip:ramtest1@talkpad.ru>";
	//	username = "ramtest1";
	//	pass = "ramtest1";
	//}
	//else if(username == "spendtime" || username == "rvoip-2")
	//{
	//	sipUri = "\"ramtest2\" <sip:ramtest2@talkpad.ru>";
	//	username = "ramtest2";
	//	pass = "ramtest2";
	//}

	//QString res;
	//res += "username: " + username + " pass: " + pass + " sipUri: " + sipUri;

	//QMessageBox::information(NULL, "debug", res);

	FSipPhoneProxy = new SipPhoneProxy(hostAddress, sipUri, username, pass, this);
	if(FSipPhoneProxy)
	{
		FSipPhoneProxy->initRegistrationData();
		connect(this, SIGNAL(sipSendInvite(const QString &)), FSipPhoneProxy, SLOT(makeNewCall(const QString&)));
		//connect(FSipPhoneProxy, SIGNAL(), this, SLOT());
		connect(this, SIGNAL(sipSendUnRegister()), FSipPhoneProxy, SLOT(makeClearRegisterProxySlot()));
		connect(FSipPhoneProxy, SIGNAL(callDeletedProxy(bool)), this, SLOT(sipCallDeletedSlot(bool)));
	}
	connect(this, SIGNAL(streamRemoved(const QString&)), this, SLOT(sipClearRegistration(const QString&)));
	connect(this, SIGNAL(streamCreated(const QString&)), this, SLOT(onStreamCreated(const QString&)));
	
	
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
				emit sipSendInvite(uri);
				// 2) Получение акцепта на запрос INVITE
				// 3) Установка соединения
			}
			else
			{
				// Пользователь отказался принимать звонок
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
}

void SipPhone::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FOpenRequests.contains(AStanzaId))
	{
		// Удаленная сторона не ответила на звонок, закрываем соединение
		QString sid = FOpenRequests.take(AStanzaId);
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
	return FDiscovery==NULL || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_RAMBLER_SIP_PHONE);
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

QString SipPhone::openStream(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FStanzaProcessor && isSupported(AStreamJid,AContactJid))
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
		//QMessageBox::information(NULL, "debug", "sipActionAfterRegistrationAsInitiator:: false");
		QMessageBox::information(NULL, "SIP Reistration", "SIP registration failed.");
		// НОТИФИКАЦИЯ О НЕУДАЧНОЙ РЕГИСТРАЦИИ
	}
}

//void SipPhone::sipRegisterInitiatorSlot(const Jid& AStreamJid, const Jid& AContactJid)
//{
//	sipActionAfterRegistrationAsInitiator(true, AStreamJid, AContactJid);
//}



// Responder part
bool SipPhone::acceptStream(const QString &AStreamId)
{
	if (FStanzaProcessor && FPendingRequests.contains(AStreamId))
	{
		connect(this, SIGNAL(sipSendRegisterAsResponder(const QString&)),
			FSipPhoneProxy, SLOT(makeRegisterResponderProxySlot(const QString&)));

		connect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const QString&)),
			this, SLOT(sipActionAfterRegistrationAsResponder(bool, const QString&)));

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

		if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
		{
			FPendingRequests.remove(AStreamId);
			stream.state = ISipStream::SS_OPENED;
			removeNotify(AStreamId);
			emit streamStateChanged(AStreamId, stream.state);
			//return true;
		}
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
		notify.data.insert(NDR_POPUP_CAPTION,message);
		notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AStream.contactJid));
		notify.data.insert(NDR_POPUP_TITLE,name);
		notify.data.insert(NDR_POPUP_STYLEKEY,STS_SIPPHONE_NOTIFYWIDGET_CALL);
		notify.data.insert(NDR_POPUP_TIMEOUT,0);
		notify.data.insert(NDR_SOUND_FILE,SDF_SIPPHONE_CALL);

		Action *acceptCall = new Action(this);
		acceptCall->setText(tr("Accept"));
		acceptCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(acceptCall,SIGNAL(triggered(bool)),SLOT(onAcceptStreamByAction(bool)));
		notify.actions.append(acceptCall);

		Action *declineCall = new Action(this);
		declineCall->setText(tr("Decline"));
		declineCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(declineCall,SIGNAL(triggered(bool)),SLOT(onCloseStreamByAction(bool)));
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
		openStream(streamJid,contactJid);
	}
}

void SipPhone::onAcceptStreamByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		acceptStream(streamId);
	}
}

void SipPhone::onCloseStreamByAction(bool) 
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		closeStream(streamId);
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

void SipPhone::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	if (AIndex->type() == RIT_CONTACT)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_JID).toString();
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
	}
}

void SipPhone::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolTips);
	if (ALabelId==RLID_DISPLAY && AIndex->type()==RIT_CONTACT)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_JID).toString();
		if (isSupported(streamJid,contactJid))
		{
			if (findStream(streamJid,contactJid).isEmpty())
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
	}
}






Q_EXPORT_PLUGIN2(plg_sipphone, SipPhone)
