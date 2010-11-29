#include "sipphone.h"

#define SHC_SIP_REQUEST "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SIP_PHONE "']"

#define CLOSE_TIMEOUT   10000
#define REQUEST_TIMEOUT 30000

SipPhone::SipPhone()
{
	FDiscovery = NULL;
	FStanzaProcessor = NULL;

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
				emit streamCreated(sid);
			}
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
				// Вроде бы звонок принят, но ответ не верный, закрываем соединение
				closeStream(sid);
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

ISipStream SipPhone::streamById(const QString &AStreamId) const
{
	return FStreams.value(AStreamId);
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
		Stanza opened("iq");
		opened.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
		QDomElement openedElem = opened.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(opened.createElement("opened")).toElement();
		openedElem.setAttribute("sid",AStreamId);

		ISipStream &stream = FStreams[AStreamId];
		if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
		{
			FPendingRequests.remove(AStreamId);
			stream.state = ISipStream::SS_OPENED;
			emit streamStateChanged(AStreamId);
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
			Stanza close("iq");
			close.setType("set").setId(FStanzaProcessor->newId()).setTo(stream.contactJid.eFull());
			QDomElement closeElem = close.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(close.createElement("close")).toElement();
			closeElem.setAttribute("sid",stream.sid);
			if (FStanzaProcessor->sendStanzaRequest(this,stream.streamJid,close,CLOSE_TIMEOUT))
			{
				//Отправлен запрос на закрытие соединения
				stream.state = ISipStream::SS_CLOSE;
				emit streamStateChanged(AStreamId);
			}
			else
			{
				//Не удалось отправить запрос, возможно связь с сервером прервалась, считаем сессию закрытой
				removeStream(AStreamId);
			}
			FPendingRequests.remove(AStreamId);
		}
	}
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

Q_EXPORT_PLUGIN2(plg_sipphone, SipPhone)
