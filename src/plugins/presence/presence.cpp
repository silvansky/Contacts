#include "presence.h"

#define SHC_PRESENCE  "/presence"

Presence::Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
	FStanzaProcessor = AStanzaProcessor;

	FOpened = false;
	FShow = Offline;
	FPriority = 0;

	IStanzaHandle shandle;
	shandle.handler = this;
	shandle.order = SHO_DEFAULT;
	shandle.direction = IStanzaHandle::DirectionIn;
	shandle.streamJid = FXmppStream->streamJid();
	shandle.conditions.append(SHC_PRESENCE);
	FSHIPresence = FStanzaProcessor->insertStanzaHandle(shandle);

	connect(AXmppStream->instance(),SIGNAL(error(const QString &)),SLOT(onStreamError(const QString &)));
	connect(AXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));

	LogDetaile(QString("[Presence][%1] Presence created").arg(streamJid().bare()));
}

Presence::~Presence()
{
	FStanzaProcessor->removeStanzaHandle(FSHIPresence);
}

bool Presence::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId == FSHIPresence)
	{
		int show;
		int priority;
		QString status;
		QString errCond;
		if (AStanza.type().isEmpty())
		{
			QString showText = AStanza.firstElement("show").text();
			if (showText.isEmpty())
				show = Online;
			else if (showText == "chat")
				show = Chat;
			else if (showText == "away")
				show = Away;
			else if (showText == "dnd")
				show = DoNotDisturb;
			else if (showText == "xa")
				show = ExtendedAway;
			else
				show = Online;    //Костыль под кривые клиенты и транспорты

			status = AStanza.firstElement("status").text();
			priority = AStanza.firstElement("priority").text().toInt();
		}
		else if (AStanza.type() == "unavailable")
		{
			show = Offline;
			status = AStanza.firstElement("status").text();
			priority = 0;
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			show = Error;
			status = err.message();
			errCond = err.condition();
			priority = 0;
		}
		else
			return false;

		if (AStreamJid != AStanza.from())
		{
			Jid fromJid = AStanza.from();

			IPresenceItem &pitem = FItems[fromJid];
			IPresenceItem before = pitem;

			pitem.isValid = true;
			pitem.itemJid = fromJid;
			pitem.show = show;
			pitem.priority = priority;
			pitem.status = status;
			pitem.errCondition = errCond;

			if (pitem != before)
				emit itemReceived(pitem, before);

			if (show == Offline)
				FItems.remove(fromJid);
		}
		else if (show!=IPresence::Offline && (FShow != show || FStatus != status || FPriority != priority))
		{
			LogDetaile(QString("[Presence][%1] Self presence changed by server, show=%2, status='%3'").arg(streamJid().bare()).arg(show).arg(status));
			FShow = show;
			FStatus = status;
			FPriority = priority;
			FErrCondition = errCond;
			emit changed(show,status,priority);
		}
		AAccept = true;
	}
	return false;
}

IPresenceItem Presence::presenceItem(const Jid &AItemJid) const
{
	return FItems.value(AItemJid);
}

QList<IPresenceItem> Presence::presenceItems(const Jid &AItemJid) const
{
	if (!AItemJid.isEmpty())
	{
		QList<IPresenceItem> pitems;
		foreach(IPresenceItem pitem, FItems)
			if (AItemJid && pitem.itemJid)
				pitems.append(pitem);
		return pitems;
	}
	return FItems.values();
}

Jid Presence::streamJid() const
{
	return FXmppStream->streamJid();
}

IXmppStream *Presence::xmppStream() const
{
	return FXmppStream;
}

bool Presence::isOpen() const
{
	return FOpened;
}

int Presence::show() const
{
	return FShow;
}

bool Presence::setShow(int AShow)
{
	return setPresence(AShow,FStatus,FPriority);
}

QString Presence::status() const
{
	return FStatus;
}

bool Presence::setStatus(const QString &AStatus)
{
	return setPresence(FShow,AStatus,FPriority);
}

int Presence::priority() const
{
	return FPriority;
}

bool Presence::setPriority(int APriority)
{
	return setPresence(FShow,FStatus,APriority);
}

QString Presence::errCondition() const
{
	return FErrCondition;
}

bool Presence::setPresence(int AShow, const QString &AStatus, int APriority)
{
	if (FXmppStream->isOpen() && AShow != Error)
	{
		LogDetaile(QString("[Presence][%1] Sending self presence, show=%2, status='%3'").arg(streamJid().bare()).arg(AShow).arg(AStatus));

		QString show;
		switch (AShow)
		{
		case Online:
			show = QString::null;
			break;
		case Chat:
			show = "chat";
			break;
		case Away:
			show = "away";
			break;
		case DoNotDisturb:
			show = "dnd";
			break;
		case ExtendedAway:
			show = "xa";
			break;
		case Invisible:
			show = QString::null;
			break;
		case Offline:
			show = QString::null;
			break;
		default:
			LogError(QString("[Presence][%1] Invalid presence parameters").arg(streamJid().bare()));
			return false;
		}

		Stanza pres("presence");
		if (AShow == Invisible)
		{
			pres.setType("invisible");
		}
		else if (AShow == Offline)
		{
			pres.setType("unavailable");
		}
		else
		{
			if (!show.isEmpty())
				pres.addElement("show").appendChild(pres.createTextNode(show));
			pres.addElement("priority").appendChild(pres.createTextNode(QString::number(APriority)));
		}

		if (!AStatus.isEmpty())
			pres.addElement("status").appendChild(pres.createTextNode(AStatus));

		if (FOpened && AShow==Offline)
			emit aboutToClose(AShow, AStatus);

		if (FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(), pres))
		{
			FShow = AShow;
			FStatus = AStatus;
			FPriority = APriority;
			FErrCondition.clear();

			if (!FOpened && AShow!=Offline)
			{
				FOpened = true;
				emit opened();
			}

			emit changed(FShow,FStatus,FPriority);

			if (FOpened && AShow==Offline)
			{
				clearItems();
				FOpened = false;
				emit closed();
			}
			return true;
		}
		else
		{
			LogError(QString("[Presence][%1] Failed to send self presence").arg(streamJid().bare()));
		}
	}
	else if (AShow == Offline || AShow == Error)
	{
		LogDetaile(QString("[Presence][%1] Changing self presence, show=%2, status='%3'").arg(streamJid().bare()).arg(AShow).arg(AStatus));
		
		FShow = AShow;
		FStatus = AStatus;
		FPriority = 0;
		if (FOpened)
		{
			emit aboutToClose(AShow,AStatus);
			clearItems();
			FOpened = false;
			emit closed();
		}
		emit changed(FShow,FStatus,FPriority);
		return true;
	}
	return false;
}

bool Presence::sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority)
{
	if (FXmppStream->isOpen() && AContactJid.isValid() && AContactJid!=FXmppStream->streamJid().domain())
	{
		LogDetaile(QString("[Presence][%1] Sending direct presence to '%2' show=%3, status='%4'").arg(streamJid().bare(),AContactJid.full()).arg(AShow).arg(AStatus));

		QString show;
		switch (AShow)
		{
		case Online:
			show = QString::null;
			break;
		case Chat:
			show = "chat";
			break;
		case Away:
			show = "away";
			break;
		case DoNotDisturb:
			show = "dnd";
			break;
		case ExtendedAway:
			show = "xa";
			break;
		case Invisible:
			show = QString::null;
			break;
		case Offline:
			show = QString::null;
			break;
		default:
			LogError(QString("[Presence][%1] Invalid presence parameters").arg(streamJid().bare()));
			return false;
		}

		Stanza pres("presence");
		pres.setTo(AContactJid.eFull());
		if (AShow == Invisible)
		{
			pres.setType("invisible");
		}
		else if (AShow == Offline)
		{
			pres.setType("unavailable");
		}
		else
		{
			if (!show.isEmpty())
				pres.addElement("show").appendChild(pres.createTextNode(show));
			pres.addElement("priority").appendChild(pres.createTextNode(QString::number(APriority)));
		}

		if (!AStatus.isEmpty())
			pres.addElement("status").appendChild(pres.createTextNode(AStatus));

		if (FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(), pres))
		{
			emit directSent(AContactJid,AShow,AStatus,APriority);
			return true;
		}
		else
		{
			LogError(QString("[Presence][%1] Failed to send direct presence to '%2'").arg(streamJid().bare(),AContactJid.full()));
		}
	}
	return false;
}

void Presence::clearItems()
{
	QList<Jid> items = FItems.keys();
	foreach(Jid itemJid, items)
	{
		IPresenceItem &pitem = FItems[itemJid];
		IPresenceItem before = pitem;
		pitem.show = Offline;
		pitem.priority = 0;
		pitem.status.clear();
		emit itemReceived(pitem,before);
		FItems.remove(itemJid);
	}
}

void Presence::onStreamClosed()
{
	if (isOpen())
		setPresence(Offline,QString::null,0);
}

void Presence::onStreamError(const QString &AError)
{
	setPresence(Error,AError,0);
}
