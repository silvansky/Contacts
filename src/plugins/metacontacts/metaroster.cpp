#include "metaroster.h"

#include <QFile>
#include <QDomDocument>

#define ACTION_TIMEOUT        10000
#define ROSTER_TIMEOUT        30000

#define SHC_ROSTER_RESULT     "/iq[@type='result']"
#define SHC_ROSTER_REQUEST    "/iq/query[@xmlns='" NS_JABBER_ROSTER "']"
#define SHC_METACONTACTS      "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_METACONTACTS "']"

#define MC_ACTION_CREATE      "create"
#define MC_ACTION_RENAME      "rename"
#define MC_ACTION_RELEASE     "release"
#define MC_ACTION_GROUPS      "changeGroups"
#define MC_ACTION_MERGE       "merge"
#define MC_ACTION_DELETE      "delete"

MetaRoster::MetaRoster(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IRoster *ARoster) : QObject(ARoster->instance())
{
	FRoster = ARoster;
	FMetaContacts = AMetaContacts;
	FAvatars = NULL;
	FPresence = NULL;
	FStanzaProcessor = NULL;
	initialize(APluginManager);

	FOpened = false;
	FEnabled = false;
	FSHIMetaContacts = -1;

	insertStanzaHandlers();
	onPresenceAdded(FPresence);

	LogDetaile(QString("[MetaRoster][%1] Meta-roster created").arg(streamJid().bare()));
}

MetaRoster::~MetaRoster()
{
	clearMetaContacts();
	removeStanzaHandlers();
	LogDetaile(QString("[MetaRoster][%1] Meta-roster destroyed").arg(streamJid().bare()));
}

bool MetaRoster::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMetaContacts == AHandlerId)
	{
		if (isOpen() && AStreamJid==AStanza.from())
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
			query.addElement("query",NS_RAMBLER_METACONTACTS).setAttribute("ver",FRosterVer);

			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,query,ROSTER_TIMEOUT))
			{
				FOpenRequestId = query.id();
				FRosterRequest = AStanza;
				LogDetaile(QString("[MetaRoster][%1] Initial meta-roster request sent").arg(streamJid().bare()));
			}
			else
			{
				LogError(QString("[MetaRoster][%1] Failed to send initial meta-roster request").arg(streamJid().bare()));
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
	if (FOpenRequestId == AStanza.id())
	{
		if (AStanza.type() == "result")
		{
			LogDetaile(QString("[MetaRoster][%1] Initial meta-roster request received").arg(streamJid().bare()));

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
			ErrorHandler err(AStanza.element());
			LogError(QString("[MetaRoster][%1] Failed to receive initial meta-roster: %2").arg(streamJid().bare(),err.message()));

			setEnabled(false);
			removeStanzaHandlers();
			FStanzaProcessor->sendStanzaOut(AStreamJid,FRosterRequest);
		}
	}
	else if (FCreateItemRequest.contains(AStanza.id()))
	{
		if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[MetaRoster][%1] Failed to create meta-item, id='%2': %3").arg(streamJid().bare(),AStanza.id(),AStanza.id(),err.message()));
			FCreateItemRequest.remove(AStanza.id());
		}
	}
	else
	{
		QString errCond;
		QString errMessage;
		if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			errCond = err.condition();
			errMessage = err.message();
		}
		processStanzaRequest(AStanza.id(),errCond,errMessage);
	}
}

void MetaRoster::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	if (FOpenRequestId == AStanzaId)
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[MetaRoster][%1] Failed to receive initial meta-roster: %2").arg(streamJid().bare(),err.message()));

		setEnabled(false);
		removeStanzaHandlers();
		FStanzaProcessor->sendStanzaOut(AStreamJid,FRosterRequest);
	}
	else if (FCreateItemRequest.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[MetaRoster][%1] Failed to create meta-item id='%2': %3").arg(streamJid().bare(),AStanzaId,err.message()));
		FCreateItemRequest.remove(AStanzaId);
	}
	else
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		processStanzaRequest(AStanzaId,err.condition(),err.message());
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

QList<QString> MetaRoster::metaContacts() const
{
	return FContacts.keys();
}

IMetaContact MetaRoster::metaContact(const QString &AMetaId) const
{
	return FContacts.value(AMetaId);
}

QString MetaRoster::itemMetaContact(const Jid &AItemJid) const
{
	return FItemMetaId.value(AItemJid.pBare());
}

IPresenceItem MetaRoster::metaPresenceItem(const QString &AMetaId) const
{
	IPresenceItem pitem;
	if (FContacts.contains(AMetaId))
	{
		pitem.isValid = true;
		if (FPresence)
		{
			bool hasNotServiceItem = false;
			QMultiMap<int, IPresenceItem> contactItems;
			QMultiMap<int, IPresenceItem> serviceItems;
			IMetaContact contact = FContacts.value(AMetaId);
			QMultiMap<int, Jid> orders = FMetaContacts->itemOrders(contact.items.toList());
			foreach(const Jid &itemJid, orders.values())
			{
				bool isService = FMetaContacts->metaDescriptorByItem(itemJid).service;
				foreach(IPresenceItem item_pres, FPresence->presenceItems(itemJid))
				{
					if (isService)
						serviceItems.insertMulti(item_pres.show,item_pres);
					else
						contactItems.insertMulti(item_pres.show,item_pres);
				}
				hasNotServiceItem |= !isService;
			}
			if (!contactItems.isEmpty())
				pitem = contactItems.constBegin().value();
			else if (!hasNotServiceItem && !serviceItems.isEmpty())
				pitem = serviceItems.constBegin().value();
		}
	}
	return pitem;
}

QList<IPresenceItem> MetaRoster::itemPresences(const Jid &AItemJid) const
{
	return FPresence!=NULL ? FPresence->presenceItems(AItemJid) : QList<IPresenceItem>();
}

QString MetaRoster::metaAvatarHash(const QString &AMetaId) const
{
	QString hash;
	if (FAvatars && FContacts.contains(AMetaId))
	{
		IMetaContact contact = FContacts.value(AMetaId);
		QMultiMap<int, Jid> orders = FMetaContacts->itemOrders(contact.items.toList());
		for (QMultiMap<int, Jid>::const_iterator it=orders.constBegin(); hash.isEmpty() && it!=orders.constEnd(); it++)
			hash = FAvatars->avatarHash(it.value());
	}
	return hash;
}

QImage MetaRoster::metaAvatarImage(const QString &AMetaId, bool AAllowNull, bool AAllowGray) const
{
	QImage image;
	if (FAvatars && FContacts.contains(AMetaId))
	{
		IMetaContact contact = FContacts.value(AMetaId);
		QMultiMap<int, Jid> orders = FMetaContacts->itemOrders(contact.items.toList());
		for (QMultiMap<int, Jid>::const_iterator it=orders.constBegin(); image.isNull() && it!=orders.constEnd(); it++)
			image = FAvatars->avatarImage(it.value(),true,false);

		if (AAllowGray && !image.isNull())
		{
			IPresenceItem pitem = metaPresenceItem(AMetaId);
			if (pitem.show==IPresence::Offline || pitem.show==IPresence::Error)
				image = ImageManager::opacitized(image);
		}
		else if (!AAllowNull && image.isNull())
		{
			image = FAvatars->avatarImage(orders.constBegin().value(),false,AAllowGray);
		}
	}
	return image;
}

QSet<QString> MetaRoster::groups() const
{
	QSet<QString> allGroups;
	for (QHash<QString, IMetaContact>::const_iterator it = FContacts.constBegin(); it!=FContacts.constEnd(); it++)
		allGroups += it->groups;
	return allGroups;
}

QList<IMetaContact> MetaRoster::groupContacts(const QString &AGroup) const
{
	QList<IMetaContact> contacts;
	QString groupWithDelim = AGroup+roster()->groupDelimiter();
	for (QHash<QString, IMetaContact>::const_iterator it = FContacts.constBegin(); it!=FContacts.constEnd(); it++)
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

void MetaRoster::saveMetaContacts(const QString &AFileName) const
{
	if (isEnabled())
	{
		QDomDocument xml;
		QDomElement metasElem = xml.appendChild(xml.createElement("metacontacts")).toElement();
		metasElem.setAttribute("ver",FRosterVer);
		metasElem.setAttribute("streamJid",streamJid().pBare());
		metasElem.setAttribute("groupDelimiter",roster()->groupDelimiter());
		foreach(IMetaContact contact, FContacts)
		{
			QDomElement mcElem = metasElem.appendChild(xml.createElement("mc")).toElement();
			mcElem.setAttribute("id",contact.id);
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
			LogDetaile(QString("[MetaRoster][%1] Meta-roster saved to file '%2'").arg(streamJid().bare(),AFileName));
		}
		else
		{
			LogError(QString("[MetaRoster][%1] Failed to save meta-roster to file '%2'").arg(streamJid().bare(),AFileName));
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
					LogDetaile(QString("[MetaRoster][%1] Loading meta-roster from file '%2'").arg(streamJid().bare(),AFileName));
					setEnabled(true);
					processRosterStanza(streamJid(),convertMetaElemToRosterStanza(metasElem));
					processMetasElement(metasElem,true);
				}
			}
			metaFile.close();
		}
	}
}

QString MetaRoster::createContact(const IMetaContact &AContact)
{
	if (isOpen())
	{
		LogDetaile(QString("[MetaRoster][%1] Creating new meta-contact with %2 item(s)").arg(streamJid().bare()).arg(AContact.items.count()));

		QSet<Jid> mergeList;
		QList<QString> requests;
		foreach(Jid itemJid, AContact.items)
		{
			if (!FItemMetaId.contains(itemJid.bare()))
			{
				Stanza query("iq");
				query.setType("set").setId(FStanzaProcessor->newId());

				QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
				mcElem.setAttribute("action",MC_ACTION_CREATE);
				mcElem.setAttribute("name",AContact.name);
				mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",itemJid.eBare());

				foreach(QString group, AContact.groups)
					if (!group.isEmpty())
						mcElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(group));

				if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
				{
					mergeList += itemJid.bare();
					requests.append(query.id());
					FCreateItemRequest.insert(query.id(),itemJid);
					LogDetaile(QString("[MetaRoster][%1] Create meta-item '%2' request sent, id='%3'").arg(streamJid().bare(),itemJid.bare(),query.id()));
				}
				else
				{
					LogError(QString("[MetaRoster][%1] Failed to send create meta-item '%2' request").arg(streamJid().bare(),itemJid.bare()));
				}
			}
			else
			{
				LogError(QString("[MetaRoster][%1] Meta-item '%2' is already present in another meta-contact").arg(streamJid().bare(),itemJid.bare()));
			}
		}

		QString multiId = startMultiRequest(requests);
		if (!multiId.isEmpty())
		{
			if (mergeList.count() > 1)
			{
				FCreateMergeList.insert(multiId,mergeList);
				LogDetaile(QString("[MetaRoster][%1] Created merge list with %2 items, id='%3'").arg(streamJid().bare()).arg(mergeList.count()).arg(multiId));
			}
		}
		else
		{
			LogError(QString("[MetaRoster][%1] Failed to create new meta-contact").arg(streamJid().bare()));
		}

		return multiId;
	}
	return QString::null;
}

QString MetaRoster::renameContact(const QString &AMetaId, const QString &ANewName)
{
	if (isOpen() && FContacts.contains(AMetaId))
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());

		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action",MC_ACTION_RENAME);
		mcElem.setAttribute("id",AMetaId);
		mcElem.setAttribute("name",ANewName);

		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			LogDetaile(QString("[MetaRoster][%1] Rename meta-contact '%2' request sent, id='%3'").arg(streamJid().bare(),AMetaId,query.id()));
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::deleteContact(const QString &AMetaId)
{
	if (isOpen() && FContacts.contains(AMetaId))
	{
		IMetaContact contact = FContacts.value(AMetaId);
		LogDetaile(QString("[MetaRoster][%1] Deleting meta-contact '%2' with %3 item(s)").arg(streamJid().bare(),AMetaId).arg(contact.items.count()));
		
		QList<QString> requests;
		foreach(Jid itemJid, contact.items)
		{
			Stanza query("iq");
			query.setType("set").setId(FStanzaProcessor->newId());

			QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
			mcElem.setAttribute("action",MC_ACTION_DELETE);
			mcElem.setAttribute("id",AMetaId);
			mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",itemJid.eBare());

			if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
			{
				requests.append(query.id());
				LogDetaile(QString("[MetaRoster][%1] Delete meta-item '%2' request sent, id='%3'").arg(streamJid().bare(),itemJid.bare(),query.id()));
			}
			else
			{
				LogError(QString("[MetaRoster][%1] Failed to send delete meta-item '%2' request.").arg(streamJid().bare(),itemJid.bare()));
			}
		}

		if (requests.isEmpty())
			LogError(QString("[MetaRoster][%1] Failed to delete meta-contact '%2'").arg(streamJid().bare(),AMetaId));

		return startMultiRequest(requests);;
	}
	return QString::null;
}

QString MetaRoster::mergeContacts(const QString &AParentId, const QList<QString> &AChildsId)
{
	if (isOpen() && FContacts.contains(AParentId) && !AChildsId.isEmpty())
	{
		LogDetaile(QString("[MetaRoster][%1] Merging meta-contact '%2' with another %3 meta-contact(s)").arg(streamJid().bare(),AParentId).arg(AChildsId.count()));

		QList<QString> requests;
		foreach(QString metaId, AChildsId)
		{
			Stanza query("iq");
			query.setType("set").setId(FStanzaProcessor->newId());

			QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
			mcElem.setAttribute("action",MC_ACTION_MERGE);
			mcElem.setAttribute("id",AParentId);

			QDomElement metaElem = mcElem.appendChild(query.createElement("mc")).toElement();
			metaElem.setAttribute("id",metaId);

			if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
			{
				requests.append(query.id());
				LogDetaile(QString("[MetaRoster][%1] Merge meta-contact with '%2' request sent, id='%3'").arg(streamJid().bare(),metaId,query.id()));
			}
			else
			{
				LogError(QString("[MetaRoster][%1] Failed to send merge meta-contact with '%2' request").arg(streamJid().bare(),metaId));
			}
		}

		if (requests.isEmpty())
			LogError(QString("[MetaRoster][%1] Failed to merge meta-contact '%2' with another contacts").arg(streamJid().bare(),AParentId));

		return startMultiRequest(requests);
	}
	return QString::null;
}

QString MetaRoster::setContactGroups(const QString &AMetaId, const QSet<QString> &AGroups)
{
	IMetaContact contact = FContacts.value(AMetaId);
	if (isOpen() && !contact.id.isEmpty() && contact.groups!=AGroups)
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());

		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action",MC_ACTION_GROUPS);
		mcElem.setAttribute("id",AMetaId);

		foreach(QString group, AGroups)
			if (!group.isEmpty())
				mcElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(group));

		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			LogDetaile(QString("[MetaRoster][%1] Change meta-contact '%2' groups request sent, id='%3'").arg(streamJid().bare(),AMetaId,query.id()));
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::detachContactItem(const QString &AMetaId, const Jid &AItemJid)
{
	if (isOpen() && itemMetaContact(AItemJid)==AMetaId)
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());

		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action",MC_ACTION_RELEASE);
		mcElem.setAttribute("id",AMetaId);
		mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",AItemJid.eBare());

		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			LogDetaile(QString("[MetaRoster][%1] Detach meta-item '%2' from meta-contact '%3' request sent, id='%4'").arg(streamJid().bare(),AItemJid.bare(),AMetaId,query.id()));
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::deleteContactItem(const QString &AMetaId, const Jid &AItemJid)
{
	if (isOpen() && itemMetaContact(AItemJid)==AMetaId)
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());

		QDomElement mcElem = query.addElement("query",NS_RAMBLER_METACONTACTS).appendChild(query.createElement("mc")).toElement();
		mcElem.setAttribute("action",MC_ACTION_DELETE);
		mcElem.setAttribute("id",AMetaId);
		mcElem.appendChild(query.createElement("item")).toElement().setAttribute("jid",AItemJid.eBare());

		if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),query,ACTION_TIMEOUT))
		{
			LogDetaile(QString("[MetaRoster][%1] Delete meta-item '%2' from meta-contact '%3' request sent, id='%4'").arg(streamJid().bare(),AItemJid.bare(),AMetaId,query.id()));
			FActionRequests.append(query.id());
			return query.id();
		}
	}
	return QString::null;
}

QString MetaRoster::renameGroup(const QString &AGroup, const QString &ANewName)
{
	if (isOpen())
	{
		QList<IMetaContact> allGroupContacts = groupContacts(AGroup);
		LogDetaile(QString("[MetaRoster][%1] Renaming group '%2' with %3 meta-contact(s) to '%4'").arg(streamJid().bare()).arg(allGroupContacts.count()).arg(AGroup,ANewName));

		QList<QString> requests;
		for (QList<IMetaContact>::const_iterator it = allGroupContacts.constBegin(); it!=allGroupContacts.constEnd(); it++)
		{
			QSet<QString> newItemGroups;
			foreach(QString group, it->groups)
			{
				QString newGroup = group;
				if (newGroup.startsWith(AGroup))
				{
					newGroup.remove(0,AGroup.size());
					newGroup.prepend(ANewName);
				}
				newItemGroups += newGroup;
			}

			QString queryId = setContactGroups(it->id,newItemGroups);
			if (!queryId.isEmpty())
				requests.append(queryId);
			else
				LogError(QString("[MetaRoster][%1] Failed to send change meta-contact '%2' groups request").arg(streamJid().bare(),it->id));
		}
		
		if (requests.isEmpty())
			LogError(QString("[MetaRoster][%1] Failed to rename group '%2' to '%3'").arg(streamJid().bare(),AGroup,ANewName));

		return startMultiRequest(requests);
	}
	return QString::null;
}

void MetaRoster::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (presencePlugin)
		{
			FPresence = presencePlugin->getPresence(FRoster->streamJid());
			connect(presencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *)));
			connect(presencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
		if (FAvatars)
		{
			connect(FAvatars->instance(),SIGNAL(avatarChanged(const Jid &)),SLOT(onAvatarChanged(const Jid &)));
		}
	}

	connect(FRoster->xmppStream()->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
	connect(FRoster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),SLOT(onStreamJidAboutToBeChanged(const Jid &)));
	connect(FRoster->instance(),SIGNAL(streamJidChanged(const Jid &)),SLOT(onStreamJidChanged(const Jid &)));
}

void MetaRoster::setEnabled(bool AEnabled)
{
	if (FEnabled != AEnabled)
	{
		LogDetaile(QString("[MetaRoster][%1] Meta roster enable=%2").arg(streamJid().bare()).arg(AEnabled));
		if (!AEnabled)
			clearMetaContacts();
		FEnabled = AEnabled;
		emit metaRosterEnabled(AEnabled);
	}
}

void MetaRoster::clearMetaContacts()
{
	foreach(QString metaId, FContacts.keys()) {
		removeMetaContact(metaId); }
	FRosterVer.clear();
}

void MetaRoster::removeMetaContact(const QString &AMetaId)
{
	IMetaContact contact = FContacts.take(AMetaId);
	IMetaContact before = contact;
	foreach(Jid itemJid, contact.items)
		if (FItemMetaId.value(itemJid) == AMetaId)
			FItemMetaId.remove(itemJid);
	contact.items.clear();
	contact.groups.clear();
	contact.name.clear();
	emit metaContactReceived(contact,before);
}

IRosterItem MetaRoster::metaRosterItem(const QSet<Jid> AItems) const
{
	IRosterItem ritem;

	QList<Jid> contactItems;
	QList<Jid> serviceItems;
	for (QSet<Jid>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); it++)
	{
		if (FMetaContacts->metaDescriptorByItem(*it).service)
			serviceItems.append(*it);
		else
			contactItems.append(*it);
	}

	QList<Jid> items = FMetaContacts->itemOrders(contactItems.isEmpty() ? serviceItems : contactItems).values();
	for (QList<Jid>::const_iterator it=items.constBegin(); it!=items.constEnd(); it++)
	{
		IRosterItem childRosterItem = roster()->rosterItem(*it);
		if (ritem.ask.isEmpty() && !childRosterItem.ask.isEmpty())
			ritem.ask = childRosterItem.ask;
		if (ritem.subscription == SUBSCRIPTION_NONE)
			ritem.subscription = childRosterItem.subscription;
		else if (childRosterItem.subscription == SUBSCRIPTION_BOTH)
			ritem.subscription = childRosterItem.subscription;
	}

	return ritem;
}

void MetaRoster::processMetasElement(QDomElement AMetasElement, bool ACompleteRoster)
{
	if (!AMetasElement.isNull())
	{
		FRosterVer = AMetasElement.attribute("ver");
		QSet<QString> oldContacts = ACompleteRoster ? FContacts.keys().toSet() : QSet<QString>();
		QDomElement mcElem = AMetasElement.firstChildElement("mc");
		while (!mcElem.isNull())
		{
			QString metaId = mcElem.attribute("id");
			QString action = mcElem.attribute("action");
			if (!metaId.isEmpty() && action.isEmpty())
			{
				IMetaContact &contact = FContacts[metaId];
				IMetaContact before = contact;

				contact.id = metaId;
				contact.name = mcElem.attribute("name");
				oldContacts -= metaId;

				contact.items.clear();
				QSet<QString> modContacts;
				QDomElement metaItem = mcElem.firstChildElement("item");
				while (!metaItem.isNull())
				{
					Jid itemJid = Jid(metaItem.attribute("jid")).pBare();
					QString prevMetaId = FItemMetaId.value(itemJid);
					if (!prevMetaId.isEmpty() && prevMetaId!=metaId)
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
					QString group = groupElem.text();
					if (!group.isEmpty())
						contact.groups += group;
					groupElem = groupElem.nextSiblingElement("group");
				}

				foreach(QString modMetaId, modContacts)
				{
					IMetaContact &modContact = FContacts[modMetaId];
					IMetaContact modBefore = modContact;
					modContact.items -= contact.items;

					IRosterItem ritem = metaRosterItem(modContact.items);
					modContact.ask = ritem.ask;
					modContact.subscription = ritem.subscription;

					if (!modContact.items.isEmpty())
						emit metaContactReceived(modContact,modBefore);
					else
						removeMetaContact(modMetaId);
				}

				IRosterItem ritem = metaRosterItem(contact.items);
				contact.ask = ritem.ask;
				contact.subscription = ritem.subscription;

				if (contact != before)
					emit metaContactReceived(contact,before);

				if (!FCreateItemRequest.isEmpty())
				{
					for(QSet<Jid>::const_iterator it=contact.items.constBegin(); it!=contact.items.constEnd(); it++)
					{
						QString requestId = FCreateItemRequest.key(*it);
						if (!requestId.isEmpty())
						{
							FCreateItemRequest.remove(requestId);
							processStanzaRequest(requestId,QString::null,QString::null);
						}
					}
				}
			}
			else if (FContacts.contains(metaId))
			{
				IMetaContact &contact = FContacts[metaId];
				IMetaContact before = contact;

				if (action == MC_ACTION_RENAME)
				{
					contact.name = mcElem.attribute("name");
					emit metaContactReceived(contact,before);
				}
				else if (action == MC_ACTION_GROUPS)
				{
					contact.groups.clear();
					QDomElement groupElem = mcElem.firstChildElement("group");
					while (!groupElem.isNull())
					{
						contact.groups += groupElem.text();
						groupElem = groupElem.nextSiblingElement("group");
					}
					emit metaContactReceived(contact,before);
				}
				else if (action == MC_ACTION_DELETE)
				{
					QDomElement metaItem = mcElem.firstChildElement("item");
					if (!metaItem.isNull())
					{
						while(!metaItem.isNull())
						{
							Jid itemJid = Jid(metaItem.attribute("jid")).pBare();
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

		foreach(QString metaId, oldContacts) {
			removeMetaContact(metaId); }
	}
}

Stanza MetaRoster::convertMetaElemToRosterStanza(QDomElement AMetaElem) const
{
	Stanza iq("iq");
	iq.setType("set").setFrom(streamJid().eFull()).setId(FStanzaProcessor->newId());

	if (!AMetaElem.isNull())
	{
		QDomElement queryElem = iq.element().appendChild(iq.createElement("query",NS_JABBER_ROSTER)).toElement();
		if (!FRosterVer.isEmpty())
			queryElem.setAttribute("ver",FRosterVer);

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
			else if (action == MC_ACTION_RENAME)
			{
				IMetaContact contact = metaContact(mcElem.attribute("id"));
				foreach(Jid itemJid, contact.items)
				{
					IRosterItem ritem = FRoster->rosterItem(itemJid);
					if (ritem.isValid)
					{
						QDomElement rosterItem = queryElem.appendChild(iq.createElement("item")).toElement();
						rosterItem.setAttribute("jid",itemJid.eBare());
						rosterItem.setAttribute("name",mcElem.attribute("name"));
						rosterItem.setAttribute("subscription",ritem.subscription);
						rosterItem.setAttribute("ask",ritem.ask);

						foreach(QString group, ritem.groups)
							rosterItem.appendChild(iq.createElement("group")).appendChild(iq.createTextNode(group));
					}
				}
			}
			else if (action == MC_ACTION_GROUPS)
			{
				IMetaContact contact = metaContact(mcElem.attribute("id"));
				foreach(Jid itemJid, contact.items)
				{
					IRosterItem ritem = FRoster->rosterItem(itemJid);
					if (ritem.isValid)
					{
						QDomElement rosterItem = queryElem.appendChild(iq.createElement("item")).toElement();
						rosterItem.setAttribute("jid",itemJid.eBare());
						rosterItem.setAttribute("name",ritem.name);
						rosterItem.setAttribute("subscription",ritem.subscription);
						rosterItem.setAttribute("ask",ritem.ask);

						QDomElement groupElem = mcElem.firstChildElement("group");
						while (!groupElem.isNull())
						{
							rosterItem.appendChild(iq.createElement("group")).appendChild(iq.createTextNode(groupElem.text()));
							groupElem = groupElem.nextSiblingElement("group");
						}
					}
				}
			}
			else if (action == MC_ACTION_DELETE)
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
						rosterItem.setAttribute("jid",itemJid.eBare());
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
	iq.setType("set").setFrom(streamJid().eFull()).setId(FStanzaProcessor->newId());
	QDomElement queryElem = iq.element().appendChild(iq.createElement("query",NS_RAMBLER_METACONTACTS)).toElement();

	if (!ARosterElem.isNull())
	{
		QDomElement itemElem = ARosterElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			Jid itemJid = itemElem.attribute("jid");
			QString metaId = itemMetaContact(itemJid);
			IRosterItem ritem = FRoster->rosterItem(itemJid);

			// Удаление контакта
			if (itemElem.attribute("subscription") == SUBSCRIPTION_REMOVE)
			{
				QDomElement mcElem = queryElem.appendChild(iq.createElement("mc")).toElement();
				mcElem.setAttribute("action",MC_ACTION_DELETE);
				mcElem.setAttribute("id",metaId);
				mcElem.appendChild(iq.createElement("item")).toElement().setAttribute("jid",itemJid.eBare());
			}
			// Добавление нового контакта
			else if (!ritem.isValid)
			{
				QDomElement mcElem = queryElem.appendChild(iq.createElement("mc")).toElement();
				mcElem.setAttribute("action",MC_ACTION_CREATE);
				mcElem.setAttribute("name",itemElem.attribute("name"));
				mcElem.appendChild(iq.createElement("item")).toElement().setAttribute("jid",itemJid.eBare());

				QDomElement itemGroupElem = itemElem.firstChildElement("group");
				while (!itemGroupElem.isNull())
				{
					mcElem.appendChild(iq.createElement("group")).appendChild(iq.createTextNode(itemGroupElem.text()));
					itemGroupElem = itemGroupElem.nextSiblingElement("group");
				}
			}
			// Переименование контакта
			else if (ritem.name != itemElem.attribute("name"))
			{
				QDomElement mcElem = queryElem.appendChild(iq.createElement("mc")).toElement();
				mcElem.setAttribute("action",MC_ACTION_RENAME);
				mcElem.setAttribute("id",metaId);
				mcElem.setAttribute("name",itemElem.attribute("name"));
			}

			itemElem = itemElem.nextSiblingElement("item");
		}
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
	if (FStanzaProcessor && FSHIMetaContacts < 0)
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
	if (FStanzaProcessor && FSHIMetaContacts > 0)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMetaContacts);
		FStanzaProcessor->removeStanzaHandle(FSHIRosterRequest);
		FStanzaProcessor->removeStanzaHandle(FSHIRosterResult);
		FSHIMetaContacts = -1;
	}
}

bool MetaRoster::processCreateMerge(const QString &AMultiId)
{
	if (FCreateMergeList.contains(AMultiId))
	{
		LogDetaile(QString("[MetaRoster][%1] Processing create merge list, id='%2'").arg(streamJid().bare(),AMultiId));

		QList<QString> requests;
		QString parentMetaId;
		QSet<Jid> items = FCreateMergeList.take(AMultiId);
		foreach(Jid itemJid, items)
		{
			QString metaId = itemMetaContact(itemJid);
			if (!metaId.isEmpty())
			{
				if (!parentMetaId.isEmpty())
				{
					QString requestId = mergeContacts(parentMetaId,QList<QString>()<<metaId);
					if (!requestId.isEmpty())
					{
						requests.append(requestId);
						FActionRequests.removeAll(requestId);
					}
				}
				else
				{
					parentMetaId = metaId;
				}
			}
		}

		if (!requests.isEmpty())
		{
			appendMultiRequest(AMultiId,requests);
			return true;
		}
		else
		{
			LogError(QString("[MetaRoster][%1] Failed to process create merge list, id='%2'").arg(streamJid().bare(),AMultiId));
		}
	}
	return false;
}

QString MetaRoster::startMultiRequest(const QList<QString> &AActions)
{
	if (FStanzaProcessor)
	{
		if (AActions.count() > 1)
		{
			QString multiId = FStanzaProcessor->newId();
			foreach(QString action, AActions)
				FMultiRequests.insertMulti(multiId,action);
			LogDetaile(QString("[MetaRoster][%1] Multi-request id='%2' started with actions from '%3' to '%4'").arg(streamJid().bare(),multiId,AActions.first(),AActions.last()));
			return multiId;
		}
		else if (AActions.count() == 1)
		{
			FActionRequests.append(AActions.at(0));
			return AActions.at(0);
		}
	}
	return QString::null;
}

void MetaRoster::appendMultiRequest(const QString &AMultiId, const QList<QString> &AActions)
{
	if (!AActions.isEmpty())
	{
		foreach(QString actionId, AActions)
			FMultiRequests.insertMulti(AMultiId,actionId);
		LogDetaile(QString("[MetaRoster][%1] To multi-request id='%2' appended actions from '%3' to '%4'").arg(streamJid().bare(),AMultiId,AActions.first(),AActions.last()));
	}
}

void MetaRoster::processMultiRequest(const QString &AMultiId, const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	if (FMultiRequests.contains(AMultiId,AActionId))
	{
		if (AErrMessage.isEmpty())
			LogDetaile(QString("[MetaRoster][%1] Multi-request id='%2' action id='%3' processed successfully").arg(streamJid().bare(),AMultiId,AActionId));
		else
			LogError(QString("[MetaRoster][%1] Failed to process action id='%2' of multi-request id='%3': %4").arg(streamJid().bare(),AActionId,AMultiId,AErrMessage));

		if (!AErrCond.isEmpty() && !FMultiErrors.contains(AMultiId))
			FMultiErrors.insert(AMultiId, qMakePair<QString,QString>(AErrCond,AErrMessage));
		FMultiRequests.remove(AMultiId,AActionId);

		if (!FMultiRequests.contains(AMultiId))
		{
			if (!processCreateMerge(AMultiId))
			{
				QPair<QString,QString> errPair = FMultiErrors.take(AMultiId);
				if (errPair.second.isEmpty())
					LogDetaile(QString("[MetaRoster][%1] Multi-request id='%2' processed successfully").arg(streamJid().bare(),AMultiId));
				else
					LogError(QString("[MetaRoster][%1] Failed to process multi-request id='%2': %3").arg(streamJid().bare(),AMultiId,errPair.second));
				emit metaActionResult(AMultiId, errPair.first, errPair.second);
			}
		}
	}
}

void MetaRoster::processStanzaRequest(const QString &AStanzaId, const QString &AErrCond, const QString &AErrMessage)
{
	if (FActionRequests.contains(AStanzaId))
	{
		FActionRequests.removeAll(AStanzaId);
		if (AErrMessage.isEmpty())
			LogDetaile(QString("[MetaRoster][%1] Request id='%2' processed successfully").arg(streamJid().bare(),AStanzaId));
		else
			LogError(QString("[MetaRoster][%1] Failed to process request id='%2': %3").arg(streamJid().bare(),AStanzaId,AErrMessage));
		emit metaActionResult(AStanzaId,AErrCond,AErrMessage);
	}
	else if (FMultiRequests.values().contains(AStanzaId))
	{
		QString multiId = FMultiRequests.key(AStanzaId);
		processMultiRequest(multiId,AStanzaId,AErrCond,AErrMessage);
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

void MetaRoster::onPresenceAdded(IPresence *APresence)
{
	if (APresence && APresence->streamJid()==FRoster->streamJid())
	{
		FPresence = APresence;
		connect(FPresence->instance(),SIGNAL(received(const IPresenceItem &, const IPresenceItem &)),
			SLOT(onPresenceReceived(const IPresenceItem &, const IPresenceItem &)));
	}
}

void MetaRoster::onPresenceReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	QString metaId = FItemMetaId.value(AItem.itemJid.pBare());
	if (!metaId.isEmpty())
		emit metaPresenceChanged(metaId);
}

void MetaRoster::onPresenceRemoved(IPresence *APresence)
{
	if (APresence == FPresence)
		FPresence = NULL;
}

void MetaRoster::onAvatarChanged(const Jid &AContactJid)
{
	QString metaId = FItemMetaId.value(AContactJid.pBare());
	if (!metaId.isEmpty())
		emit metaAvatarChanged(metaId);
}
