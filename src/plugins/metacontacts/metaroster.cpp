#include "metaroster.h"

#include <QFile>
#include <QDomDocument>

#define ACTION_TIMEOUT        10000
#define ROSTER_TIMEOUT        30000

#define SHC_ROSTER_RESULT     "/iq[@type='result']"
#define SHC_ROSTER_REQUEST    "/iq/query[@xmlns='" NS_JABBER_ROSTER "']"
#define SHC_METACONTACTS      "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_METACONTACTS "']"

MetaRoster::MetaRoster(IRoster *ARoster, IStanzaProcessor *AStanzaProcessor) : QObject(ARoster->instance())
{
	FRoster = ARoster;
	FStanzaProcessor = AStanzaProcessor;

	FOpened = false;
	FEnabled = false;
	FSHIMetaContacts = -1;

	connect(FRoster->xmppStream()->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
	connect(FRoster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),SLOT(onStreamJidAboutToBeChanged(const Jid &)));
	connect(FRoster->instance(),SIGNAL(streamJidChanged(const Jid &)),SLOT(onStreamJidChanged(const Jid &)));

	insertStanzaHandlers();
}

MetaRoster::~MetaRoster()
{
	clearMetaContacts();
	removeStanzaHandlers();
}

bool MetaRoster::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMetaContacts == AHandlerId)
	{
		if (isOpen() && (AStanza.from().isEmpty() || (AStreamJid && AStanza.from())))
		{
			AAccept = true;

			QDomElement metasElem = AStanza.firstElement("query",NS_RAMBLER_METACONTACTS);
			processRosterStanza(AStreamJid,convertMetaElemToRosterStanza(metasElem));
			processMetasElement(metasElem,false);

			Stanza result("iq");
			result.setType("result").setId(AStanza.id());
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (FSHIRosterRequest == AHandlerId)
	{
		AAccept = true;
		if (!FRoster->isOpen() && AStanza.type()=="get")
		{
			Stanza query("iq");
			query.setType("get").setId(FStanzaProcessor->newId());
			query.addElement("query",NS_RAMBLER_METACONTACTS);

			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,query,ROSTER_TIMEOUT))
			{
				FOpenRequestId = query.id();
				FRosterRequest = AStanza;
			}
		}
		else if (FRoster->isOpen() && AStanza.type()=="set")
		{
			Stanza metaStanza = convertRosterElemToMetaStanza(AStanza.firstElement("query",NS_JABBER_ROSTER));
			if (metaStanza.firstElement("query",NS_RAMBLER_METACONTACTS).hasChildNodes())
				FStanzaProcessor->sendStanzaOut(AStreamJid,metaStanza);
		}
		return true;
	}
	else if (FSHIRosterResult == AHandlerId)
	{
		if (FBlockResults.contains(AStanza.id()))
		{
			FBlockResults.removeAll(AStanza.id());
			return true;
		}
	}
	return false;
}

void MetaRoster::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FActionRequests.contains(AStanza.id()))
	{
		QString errCond;
		QString errMessage;
		if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			errCond = err.condition();
			errMessage = err.message();
		}
		FActionRequests.removeAll(AStanza.id());
		emit metaActionResult(AStanza.id(),errCond,errMessage);
	}
	else if (FOpenRequestId == AStanza.id())
	{
		if (AStanza.type() == "result")
		{
			setEnabled(true);
			QDomElement metasElem = AStanza.firstElement("query",NS_RAMBLER_METACONTACTS);

			Stanza rosterStanza = convertMetaElemToRosterStanza(metasElem);
			rosterStanza.setType("result").setId(FRosterRequest.id());
			processRosterStanza(AStreamJid,rosterStanza);

			processMetasElement(metasElem,true);
			FOpened = true;
			emit metaRosterOpened();
		}
		else
		{
			setEnabled(false);
			removeStanzaHandlers();
			FStanzaProcessor->sendStanzaOut(AStreamJid,FRosterRequest);
		}
	}
}

void MetaRoster::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	if (FActionRequests.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		FActionRequests.removeAll(AStanzaId);
		emit metaActionResult(AStanzaId,err.condition(),err.message());
	}
	else if (FOpenRequestId == AStanzaId)
	{
		setEnabled(false);
		removeStanzaHandlers();
		FStanzaProcessor->sendStanzaOut(AStreamJid,FRosterRequest);
	}
}

bool MetaRoster::isEnabled() const
{
	return FEnabled;
}

Jid MetaRoster::streamJid() const
{
	return roster()->streamJid();
}

IRoster *MetaRoster::roster() const
{
	return FRoster;
}

bool MetaRoster::isOpen() const
{
	return FOpened;
}

QList<Jid> MetaRoster::metaContacts() const
{
	return FMetaContacts.keys();
}

Jid MetaRoster::itemMetaContact(const Jid &AItemJid) const
{
	return FItemMetaId.value(AItemJid.bare());
}

IMetaContact MetaRoster::metaContact(const Jid &AMetaId) const
{
	return FMetaContacts.value(AMetaId);
}

QSet<QString> MetaRoster::groups() const
{
	QSet<QString> allGroups;
	for (QHash<Jid, IMetaContact>::const_iterator it = FMetaContacts.constBegin(); it!=FMetaContacts.constEnd(); it++)
		allGroups += it->groups;
	return allGroups;
}

QList<IMetaContact> MetaRoster::groupContacts(const QString &AGroup) const
{
	QList<IMetaContact> contacts;
	QString groupWithDelim = AGroup+roster()->groupDelimiter();
	for (QHash<Jid, IMetaContact>::const_iterator it = FMetaContacts.constBegin(); it!=FMetaContacts.constEnd(); it++)
	{
		foreach(QString group, it->groups)
		{
			if (group==AGroup || group.startsWith(AGroup))
			{
				contacts.append(it.value());
				break;
			}
		}
	}
	return contacts;
}

QSet<QString> MetaRoster::contactGroups(const Jid &AMetaId) const
{
	return FMetaContacts.value(AMetaId).groups;
}

QString MetaRoster::setContactGroups(const Jid &AMetaId, const QSet<QString> &AGroups)
{
	IMetaContact contact = FMetaContacts.value(AMetaId);
	if (isOpen() && isEnabled() && contact.id.isValid() && contact.groups!=AGroups)
	{
		/*
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();

		mcElem.setAttribute("id",contact.id.full());
		mcElem.setAttribute("name",contact.name);

		foreach(Jid itemJid, contact.items)
		{
			IRosterItem ritem = FRoster->rosterItem(itemJid);
			QDomElement itemElem = mcElem.appendChild(query.createElement("item")).toElement();
			itemElem.setAttribute("jid",itemJid.eFull());
			itemElem.setAttribute("subscription",ritem.subscription);
			itemElem.setAttribute("ask",ritem.ask);
		}

		foreach(QString group, AGroups)
		{
			mcElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(group));
		}

		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
		*/
	}
	return QString::null;
}

QString MetaRoster::releaseContactItem(const Jid &AMetaId, const Jid &AItemJid)
{
	if (isOpen() && isEnabled() && itemMetaContact(AItemJid)==AMetaId)
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action","release");
		mcElem.setAttribute("id",AMetaId.eFull());
		mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",AItemJid.eFull());
		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::deleteContactItem(const Jid &AMetaId, const Jid &AItemJid)
{
	if (isOpen() && isEnabled() && itemMetaContact(AItemJid)==AMetaId)
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action","delete");
		mcElem.setAttribute("id",AMetaId.eFull());
		mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",AItemJid.eFull());
		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::renameContact(const Jid &AMetaId, const QString &ANewName)
{
	if (isOpen() && isEnabled() && FMetaContacts.contains(AMetaId))
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action","rename");
		mcElem.setAttribute("id",AMetaId.eFull());
		mcElem.setAttribute("name",ANewName);
		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::deleteContact(const Jid &AMetaId)
{
	if (isOpen() && isEnabled() && FMetaContacts.contains(AMetaId))
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action","delete");
		mcElem.setAttribute("id",AMetaId.eFull());
		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::mergeContacts(const Jid &AParentId, const QList<Jid> &AChildsId)
{
	if (isOpen() && isEnabled() && FMetaContacts.contains(AParentId) && !AChildsId.isEmpty())
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action","merge");
		mcElem.setAttribute("id",AParentId.eFull());
		foreach(Jid metaId, AChildsId)
			mcElem.appendChild(query.createElement("mc")).toElement().setAttribute("id",metaId.eFull());
		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

void MetaRoster::saveMetaContacts(const QString &AFileName) const
{
	if (isEnabled())
	{
		QDomDocument xml;
		QDomElement metasElem = xml.appendChild(xml.createElement("metacontacts")).toElement();
		metasElem.setAttribute("streamJid",streamJid().pBare());
		metasElem.setAttribute("groupDelimiter",roster()->groupDelimiter());
		foreach(IMetaContact contact, FMetaContacts)
		{
			QDomElement mcElem = metasElem.appendChild(xml.createElement("mc")).toElement();
			mcElem.setAttribute("id",contact.id.eBare());
			mcElem.setAttribute("name",contact.name);
			foreach(Jid itemJid, contact.items)
			{
				IRosterItem ritem = roster()->rosterItem(itemJid);
				QDomElement itemElem = mcElem.appendChild(xml.createElement("item")).toElement();
				itemElem.setAttribute("jid", itemJid.eBare());
				itemElem.setAttribute("subscription",ritem.subscription);
				itemElem.setAttribute("ask",ritem.ask);
			}
			foreach(QString group, contact.groups)
			{
				mcElem.appendChild(xml.createElement("group")).appendChild(xml.createTextNode(group));
			}
		}

		QFile metaFile(AFileName);
		if (metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			metaFile.write(xml.toByteArray());
			metaFile.close();
		}
	}
}

void MetaRoster::loadMetaContacts(const QString &AFileName)
{
	if (!isOpen())
	{
		QFile metaFile(AFileName);
		if (metaFile.exists() && metaFile.open(QIODevice::ReadOnly))
		{
			QDomDocument xml;
			if (xml.setContent(metaFile.readAll()))
			{
				QDomElement metasElem = xml.firstChildElement("metacontacts");
				if (!metasElem.isNull() && metasElem.attribute("streamJid")==streamJid().pBare() && metasElem.attribute("groupDelimiter")==roster()->groupDelimiter())
				{
					setEnabled(true);
					processRosterStanza(streamJid(),convertMetaElemToRosterStanza(metasElem));
					processMetasElement(metasElem,true);
				}
			}
			metaFile.close();
		}
	}
}

void MetaRoster::setEnabled(bool AEnabled)
{
	if (FEnabled != AEnabled)
	{
		if (!AEnabled)
			clearMetaContacts();
		FEnabled = AEnabled;
		emit metaRosterEnabled(AEnabled);
	}
}

void MetaRoster::clearMetaContacts()
{
	foreach(Jid metaId, FMetaContacts.keys()) {
		removeMetaContact(metaId); }
}

void MetaRoster::removeMetaContact(const Jid &AMetaId)
{
	IMetaContact contact = FMetaContacts.take(AMetaId);
	IMetaContact before = contact;
	foreach(Jid itemJid, contact.items)
		if (FItemMetaId.value(itemJid) == AMetaId)
			FItemMetaId.remove(itemJid);
	contact.items.clear();
	contact.groups.clear();
	contact.name.clear();
	emit metaContactReceived(contact,before);
}

void MetaRoster::processMetasElement(QDomElement AMetasElement, bool ACompleteRoster)
{
	if (!AMetasElement.isNull())
	{
		QSet<Jid> oldContacts = ACompleteRoster ? FMetaContacts.keys().toSet() : QSet<Jid>();
		QDomElement mcElem = AMetasElement.firstChildElement("mc");
		while (!mcElem.isNull())
		{
			Jid metaId = mcElem.attribute("id");
			QString action = mcElem.attribute("action");
			if (metaId.isValid() && action.isEmpty())
			{
				IMetaContact &contact = FMetaContacts[metaId];
				IMetaContact before = contact;

				contact.id = metaId;
				contact.name = mcElem.attribute("name");
				oldContacts -= metaId;

				contact.items.clear();
				QSet<Jid> modContacts;
				QDomElement metaItem = mcElem.firstChildElement("item");
				while (!metaItem.isNull())
				{
					Jid itemJid = Jid(metaItem.attribute("jid")).bare();
					Jid prevMetaId = FItemMetaId.value(itemJid);
					if (prevMetaId.isValid() && prevMetaId!=metaId)
						modContacts += prevMetaId;
					contact.items += itemJid;
					FItemMetaId.insert(itemJid,metaId);
					metaItem = metaItem.nextSiblingElement("item");
				}

				QSet<Jid> oldItems = before.items - contact.items;
				foreach(Jid itemJid, oldItems)
					FItemMetaId.remove(itemJid);

				contact.groups.clear();
				QDomElement groupElem = mcElem.firstChildElement("group");
				while (!groupElem.isNull())
				{
					contact.groups += groupElem.text();
					groupElem = groupElem.nextSiblingElement("group");
				}

				foreach(Jid modMetaId, modContacts)
				{
					IMetaContact &modContact = FMetaContacts[modMetaId];
					IMetaContact modBefore = modContact;
					modContact.items -= contact.items;
					if (!modContact.items.isEmpty())
						emit metaContactReceived(modContact,modBefore);
					else
						removeMetaContact(modMetaId);
				}

				emit metaContactReceived(contact,before);
			}
			else if (FMetaContacts.contains(metaId))
			{
				IMetaContact &contact = FMetaContacts[metaId];
				IMetaContact before = contact;

				if (action == "rename")
				{
					contact.name = mcElem.attribute("name");
					emit metaContactReceived(contact,before);
				}
				else if (action == "delete")
				{
					QDomElement metaItem = mcElem.firstChildElement("item");
					if (!metaItem.isNull())
					{
						while(!metaItem.isNull())
						{
							Jid itemJid = Jid(metaItem.attribute("jid")).bare();
							contact.items -= itemJid;
							FItemMetaId.remove(itemJid);
							metaItem = metaItem.nextSiblingElement("item");
						}
						emit metaContactReceived(contact,before);
					}
					else
					{
						oldContacts += metaId;
					}
				}
			}
			mcElem = mcElem.nextSiblingElement("mc");
		}
		
		foreach(Jid metaId, oldContacts) {
			removeMetaContact(metaId); }
	}
}

Stanza MetaRoster::convertMetaElemToRosterStanza(QDomElement AMetaElem) const
{
	Stanza iq("iq");
	iq.setType("set").setId(FStanzaProcessor->newId());
	QDomElement queryElem = iq.element().appendChild(iq.createElement("query",NS_JABBER_ROSTER)).toElement();

	if (!AMetaElem.isNull())
	{
		QDomElement mcElem = AMetaElem.firstChildElement("mc");
		while (!mcElem.isNull())
		{
			QString action = mcElem.attribute("action");
			if (action.isEmpty())
			{
				QDomElement metaItem = mcElem.firstChildElement("item");
				while (!metaItem.isNull())
				{
					QDomElement rosterItem = queryElem.appendChild(metaItem.cloneNode(true)).toElement();
					rosterItem.setAttribute("name",mcElem.attribute("name"));

					QDomElement metaGroup = mcElem.firstChildElement("group");
					while (!metaGroup.isNull())
					{
						rosterItem.appendChild(metaGroup.cloneNode(true));
						metaGroup = metaGroup.nextSiblingElement("group");
					}

					metaItem = metaItem.nextSiblingElement("item");
				}
			}
			else if (action == "rename")
			{
				IMetaContact contact = metaContact(mcElem.attribute("id"));
				foreach(Jid itemJid, contact.items)
				{
					IRosterItem ritem = FRoster->rosterItem(itemJid);
					if (ritem.isValid)
					{
						QDomElement rosterItem = queryElem.appendChild(iq.createElement("item")).toElement();
						rosterItem.setAttribute("jid",itemJid.eFull());
						rosterItem.setAttribute("name",mcElem.attribute("name"));
						rosterItem.setAttribute("subscription",ritem.subscription);
						rosterItem.setAttribute("ask",ritem.ask);

						foreach(QString group, ritem.groups)
							rosterItem.appendChild(iq.createElement("group")).appendChild(iq.createTextNode(group));
					}
				}
			}
			else if (action == "delete")
			{
				QDomElement metaItem = mcElem.firstChildElement("item");
				if (!metaItem.isNull())
				{
					while(!metaItem.isNull())
					{
						QDomElement rosterItem = queryElem.appendChild(iq.createElement("item")).toElement();
						rosterItem.setAttribute("jid",metaItem.attribute("jid"));
						rosterItem.setAttribute("subscription",SUBSCRIPTION_REMOVE);
						metaItem = metaItem.nextSiblingElement("item");
					}
				}
				else
				{
					IMetaContact contact = metaContact(mcElem.attribute("id"));
					foreach(Jid itemJid, contact.items)
					{
						QDomElement rosterItem = queryElem.appendChild(iq.createElement("item")).toElement();
						rosterItem.setAttribute("jid",itemJid.eFull());
						rosterItem.setAttribute("subscription",SUBSCRIPTION_REMOVE);
					}
				}
			}
			mcElem = mcElem.nextSiblingElement("mc");
		}
	}

	return iq;
}

Stanza MetaRoster::convertRosterElemToMetaStanza(QDomElement ARosterElem) const
{
	Stanza iq("iq");
	iq.setType("set").setId(FStanzaProcessor->newId());
	QDomElement queryElem = iq.element().appendChild(iq.createElement("query",NS_RAMBLER_METACONTACTS)).toElement();
	
	if (!ARosterElem.isNull())
	{

	}

	return iq;
}

void MetaRoster::processRosterStanza(const Jid &AStreamJid, Stanza AStanza)
{
	if (AStanza.type() == "set")
		FBlockResults.append(AStanza.id());
	FStanzaProcessor->sendStanzaIn(AStreamJid,AStanza);
}

void MetaRoster::insertStanzaHandlers()
{
	if (FSHIMetaContacts < 0)
	{
		IStanzaHandle metaHandler;
		metaHandler.handler = this;
		metaHandler.order = SHO_DEFAULT;
		metaHandler.direction = IStanzaHandle::DirectionIn;
		metaHandler.streamJid = FRoster->streamJid();
		metaHandler.conditions.append(SHC_METACONTACTS);
		FSHIMetaContacts = FStanzaProcessor->insertStanzaHandle(metaHandler);

		IStanzaHandle rosterHandle;
		rosterHandle.handler = this;
		rosterHandle.order = SHO_QO_METAROSTER;
		rosterHandle.direction = IStanzaHandle::DirectionOut;
		rosterHandle.streamJid = FRoster->streamJid();

		rosterHandle.conditions.append(SHC_ROSTER_REQUEST);
		FSHIRosterRequest = FStanzaProcessor->insertStanzaHandle(rosterHandle);

		rosterHandle.conditions.clear();
		rosterHandle.conditions.append(SHC_ROSTER_RESULT);
		FSHIRosterResult = FStanzaProcessor->insertStanzaHandle(rosterHandle);
	}
}

void MetaRoster::removeStanzaHandlers()
{
	if (FSHIMetaContacts > 0)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMetaContacts);
		FStanzaProcessor->removeStanzaHandle(FSHIRosterRequest);
		FStanzaProcessor->removeStanzaHandle(FSHIRosterResult);
		FSHIMetaContacts = -1;
	}
}

void MetaRoster::onStreamClosed()
{
	if (FOpened)
	{
		FOpened = false;
		emit metaRosterClosed();
	}
	insertStanzaHandlers();
}

void MetaRoster::onStreamJidAboutToBeChanged(const Jid &AAfter)
{
	emit metaRosterStreamJidAboutToBeChanged(AAfter);
	if (!(FRoster->streamJid() && AAfter))
		clearMetaContacts();
}

void MetaRoster::onStreamJidChanged(const Jid &ABefore)
{
	emit metaRosterStreamJidChanged(ABefore);
}
