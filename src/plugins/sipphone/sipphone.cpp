#include "sipphone.h"

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
}

SipPhone::~SipPhone()
{

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
		uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
		uchar kindDefs = INotification::RosterIcon|INotification::PopupWindow|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
		FNotifications->insertNotificator(NID_SIPPHONE_CALL,OWO_NOTIFICATIONS_SIPPHONE,QString::null,kindMask,kindDefs);
	}
	return true;
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
			}
			else
			{
				// Пользователь отказался принимать звонок
				removeStream(sid);
			}
		}
		else
		{
			// Получили ошибку, по её коду можно определить причину, уведомляем пользоователя в окне звонка и закрываем сессию
			removeStream(sid);
		}
	}
	else if (FCloseRequests.contains(AStanza.id()))
	{
		// Получили ответ на закрытие сессии, есть ошибка или нет уже не важно
		QString sid = FCloseRequests.take(AStanza.id());
		removeStream(sid);
	}
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
			return sid;
		}
	}
	return QString::null;
}

bool SipPhone::acceptStream(const QString &AStreamId)
{
	if (FStanzaProcessor && FPendingRequests.contains(AStreamId))
	{
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
			return true;
		}
	}
	return false;
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
					stream.state = ISipStream::SS_CLOSE;
					emit streamStateChanged(AStreamId,stream.state);
				}
				else
				{
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
