#include "servicediscovery.h"

#include <QDir>
#include <QFile>
#include <QCryptographicHash>

#define SHC_DISCO_INFO          "/iq[@type='get']/query[@xmlns='" NS_DISCO_INFO "']"
#define SHC_DISCO_ITEMS         "/iq[@type='get']/query[@xmlns='" NS_DISCO_ITEMS "']"
#define SHC_PRESENCE            "/presence"

#define DISCO_TIMEOUT           60000

#define ADR_STREAMJID           Action::DR_StreamJid
#define ADR_CONTACTJID          Action::DR_Parametr1
#define ADR_NODE                Action::DR_Parametr2

#define DIC_CLIENT              "client"

#define QUEUE_TIMER_INTERVAL    2000
#define QUEUE_REQUEST_WAIT      5000
#define QUEUE_REQUEST_START     QDateTime::currentDateTime().addMSecs(QUEUE_REQUEST_WAIT)

#define CAPS_DIRNAME            "caps"
#define CAPS_FILE_TAG_NAME      "capabilities"

#define CAPS_HASH_MD5           "md5"
#define CAPS_HASH_SHA1          "sha-1"

ServiceDiscovery::ServiceDiscovery()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FStanzaProcessor = NULL;
	FDataForms = NULL;

	FUpdateSelfCapsStarted = false;

	FQueueTimer.setSingleShot(false);
	FQueueTimer.setInterval(QUEUE_TIMER_INTERVAL);
	connect(&FQueueTimer,SIGNAL(timeout()),SLOT(onQueueTimerTimeout()));

	connect(this,SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
}

ServiceDiscovery::~ServiceDiscovery()
{

}

void ServiceDiscovery::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Service Discovery");
	APluginInfo->description = tr("Allows to receive information about Jabber entities");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ServiceDiscovery::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool ServiceDiscovery::initObjects()
{
	registerFeatures();
	insertDiscoHandler(this);
	return true;
}

bool ServiceDiscovery::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	bool hooked = false;
	if (FSHIPresenceOut.value(AStreamJid)==AHandlerId && !FSelfCaps.value(AStreamJid).ver.isEmpty())
	{
		QDomElement capsElem = AStanza.addElement("c",NS_ENTITY_CAPS);
		capsElem.setAttribute("node",FSelfCaps.value(AStreamJid).node);
		capsElem.setAttribute("ver",FSelfCaps.value(AStreamJid).ver);
		capsElem.setAttribute("hash",FSelfCaps.value(AStreamJid).hash);
	}
	else if (FSHIInfo.value(AStreamJid) == AHandlerId)
	{
		QDomElement query = AStanza.firstElement("query", NS_DISCO_INFO);
		IDiscoInfo dinfo = selfDiscoInfo(AStreamJid,query.attribute("node"));

		if (dinfo.error.code >= 0)
		{
			AAccept = true;
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,ErrorHandler(dinfo.error.condition,dinfo.error.code));
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
		else
		{
			AAccept = true;
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			QDomElement query = result.addElement("query",NS_DISCO_INFO);
			if (!dinfo.node.isEmpty())
				query.setAttribute("node",dinfo.node);
			discoInfoToElem(dinfo,query);
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (FSHIItems.value(AStreamJid) == AHandlerId)
	{
		QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);

		IDiscoItems ditems;
		ditems.streamJid = AStreamJid;
		ditems.contactJid = AStanza.from();
		ditems.node = query.attribute("node");
		foreach(IDiscoHandler *AHandler, FDiscoHandlers)
			AHandler->fillDiscoItems(ditems);

		if (ditems.error.code >= 0)
		{
			AAccept = true;
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,ErrorHandler(ditems.error.condition,ditems.error.code));
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
		else
		{
			AAccept = true;
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			QDomElement query = result.addElement("query",NS_DISCO_ITEMS);
			if (!ditems.node.isEmpty())
				query.setAttribute("node",ditems.node);
			foreach(IDiscoItem ditem, ditems.items)
			{
				QDomElement elem = query.appendChild(result.createElement("item")).toElement();
				elem.setAttribute("jid",ditem.itemJid.eFull());
				if (!ditem.node.isEmpty())
					elem.setAttribute("node",ditem.node);
				if (!ditem.name.isEmpty())
					elem.setAttribute("name",ditem.name);
			}
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (FSHIPresenceIn.value(AStreamJid) == AHandlerId)
	{
		if (AStanza.type().isEmpty())
		{
			Jid contactJid = AStanza.from();
			QDomElement capsElem = AStanza.firstElement("c",NS_ENTITY_CAPS);

			EntityCapabilities newCaps;
			newCaps.streamJid = AStreamJid;
			newCaps.entityJid = contactJid;
			newCaps.node = capsElem.attribute("node");
			newCaps.ver = capsElem.attribute("ver");
			newCaps.hash = capsElem.attribute("hash");

			EntityCapabilities oldCaps = FEntityCaps.value(AStreamJid).value(contactJid);
			bool capsChanged = !capsElem.isNull() && (oldCaps.ver!=newCaps.ver || oldCaps.node!=newCaps.node);

			// Some gates can send back presence from your or another connection with EntityCaps!!!
			// So we should ignore entity caps from all agents
			if (!capsElem.isNull() && contactJid.node().isEmpty())
			{
				newCaps.node.clear();
				newCaps.ver.clear();
				newCaps.hash.clear();
				capsChanged = true;
			}

			if (capsElem.isNull() || capsChanged)
			{
				if (hasEntityCaps(newCaps))
				{
					IDiscoInfo dinfo = loadEntityCaps(newCaps);
					dinfo.streamJid = AStreamJid;
					dinfo.contactJid = contactJid;
					FDiscoInfo[dinfo.streamJid][dinfo.contactJid].insert(dinfo.node,dinfo);
					emit discoInfoReceived(dinfo);
				}
				else if (capsChanged || !hasDiscoInfo(AStreamJid,contactJid))
				{
					DiscoveryRequest request;
					request.streamJid = AStreamJid;
					request.contactJid = contactJid;
					//request.node = !newCaps.hash.isEmpty() ? newCaps.node+"#"+newCaps.ver : "";
					appendQueuedRequest(QUEUE_REQUEST_START,request);
				}
				if (!capsElem.isNull() && !newCaps.node.isEmpty() && !newCaps.ver.isEmpty())
				{
					FEntityCaps[AStreamJid].insert(contactJid,newCaps);
				}
			}
		}
	}
	return hooked;
}

void ServiceDiscovery::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FInfoRequestsId.contains(AStanza.id()))
	{
		DiscoveryRequest drequest = FInfoRequestsId.take(AStanza.id());
		IDiscoInfo dinfo = parseDiscoInfo(AStanza,drequest);
		FDiscoInfo[dinfo.streamJid][dinfo.contactJid].insert(dinfo.node,dinfo);
		saveEntityCaps(dinfo);
		emit discoInfoReceived(dinfo);
	}
	else if (FItemsRequestsId.contains(AStanza.id()))
	{
		DiscoveryRequest drequest = FItemsRequestsId.take(AStanza.id());
		IDiscoItems ditems = parseDiscoItems(AStanza,drequest);
		emit discoItemsReceived(ditems);
	}
}

void ServiceDiscovery::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.node.isEmpty())
	{
		IDiscoIdentity didentity;
		didentity.category = "client";
		didentity.type = "pc";
		didentity.name = CLIENT_NAME;
		ADiscoInfo.identity.append(didentity);

		foreach(IDiscoFeature feature, FDiscoFeatures)
			if (feature.active)
				ADiscoInfo.features.append(feature.var);
	}
}

void ServiceDiscovery::fillDiscoItems(IDiscoItems &ADiscoItems)
{
	Q_UNUSED(ADiscoItems);
}

IDiscoInfo ServiceDiscovery::selfDiscoInfo(const Jid &AStreamJid, const QString &ANode) const
{
	IDiscoInfo dinfo;
	dinfo.streamJid = AStreamJid;
	dinfo.contactJid = AStreamJid;

	const EntityCapabilities myCaps = FSelfCaps.value(AStreamJid);
	QString capsNode = QString("%1#%2").arg(myCaps.node).arg(myCaps.ver);
	dinfo.node = ANode!=capsNode ? ANode : "";

	foreach(IDiscoHandler *handler, FDiscoHandlers)
		handler->fillDiscoInfo(dinfo);

	dinfo.node = ANode;

	return dinfo;
}

bool ServiceDiscovery::checkDiscoFeature(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault)
{
	IDiscoInfo dinfo = discoInfo(AStreamJid,AContactJid,ANode);
	return dinfo.error.code>=0 || !dinfo.contactJid.isValid() ? ADefault : dinfo.features.contains(AFeature);
}

QList<IDiscoInfo> ServiceDiscovery::findDiscoInfo(const Jid &AStreamJid, const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const
{
	QList<IDiscoInfo> result;
	QList<Jid> searchJids = AParent.itemJid.isValid() ? QList<Jid>()<<AParent.itemJid : FDiscoInfo.value(AStreamJid).keys();
	foreach(Jid itemJid, searchJids)
	{
		QMap<QString, IDiscoInfo> itemInfos = FDiscoInfo.value(AStreamJid).value(itemJid);
		QList<QString> searchNodes = !AParent.node.isEmpty() ? QList<QString>()<<AParent.node : itemInfos.keys();
		foreach(QString itemNode, searchNodes)
		{
			IDiscoInfo itemInfo = itemInfos.value(itemNode);
			if (compareIdentities(itemInfo.identity,AIdentity) && compareFeatures(itemInfo.features,AFeatures))
				result.append(itemInfo);
		}
	}
	return result;
}

void ServiceDiscovery::updateSelfEntityCapabilities()
{
	if (!FUpdateSelfCapsStarted)
	{
		FUpdateSelfCapsStarted = true;
		QTimer::singleShot(0,this,SLOT(onSelfCapsChanged()));
	}
}

void ServiceDiscovery::insertDiscoHandler(IDiscoHandler *AHandler)
{
	if (!FDiscoHandlers.contains(AHandler))
	{
		FDiscoHandlers.append(AHandler);
		emit discoHandlerInserted(AHandler);
	}
}

void ServiceDiscovery::removeDiscoHandler(IDiscoHandler *AHandler)
{
	if (FDiscoHandlers.contains(AHandler))
	{
		FDiscoHandlers.removeAt(FDiscoHandlers.indexOf(AHandler));
		emit discoHandlerRemoved(AHandler);
	}
}

bool ServiceDiscovery::hasFeatureHandler(const QString &AFeature) const
{
	return FFeatureHandlers.contains(AFeature);
}

void ServiceDiscovery::insertFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler, int AOrder)
{
	if (!FFeatureHandlers.value(AFeature).values().contains(AHandler))
	{
		FFeatureHandlers[AFeature].insertMulti(AOrder,AHandler);
		emit featureHandlerInserted(AFeature,AHandler);
	}
}

bool ServiceDiscovery::execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
	foreach(IDiscoFeatureHandler *handler, handlers)
		if (handler->execDiscoFeature(AStreamJid,AFeature,ADiscoInfo))
			return true;
	return false;
}

QList<Action *> ServiceDiscovery::createFeatureActions(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	QList<Action *> actions;
	QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
	foreach(IDiscoFeatureHandler *handler, handlers)
	{
		Action *action = handler->createDiscoFeatureAction(AStreamJid,AFeature,ADiscoInfo,AParent);
		if (action)
			actions.append(action);
	}
	return actions;
}

void ServiceDiscovery::removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler)
{
	if (FFeatureHandlers.value(AFeature).values().contains(AHandler))
	{
		FFeatureHandlers[AFeature].remove(FFeatureHandlers[AFeature].key(AHandler),AHandler);
		if (FFeatureHandlers.value(AFeature).isEmpty())
			FFeatureHandlers.remove(AFeature);
		emit featureHandlerRemoved(AFeature,AHandler);
	}
}

void ServiceDiscovery::insertDiscoFeature(const IDiscoFeature &AFeature)
{
	if (!AFeature.var.isEmpty())
	{
		removeDiscoFeature(AFeature.var);
		FDiscoFeatures.insert(AFeature.var,AFeature);
		emit discoFeatureInserted(AFeature);
		updateSelfEntityCapabilities();
	}
}

QList<QString> ServiceDiscovery::discoFeatures() const
{
	return FDiscoFeatures.keys();
}

IDiscoFeature ServiceDiscovery::discoFeature(const QString &AFeatureVar) const
{
	return FDiscoFeatures.value(AFeatureVar);
}

void ServiceDiscovery::removeDiscoFeature(const QString &AFeatureVar)
{
	if (FDiscoFeatures.contains(AFeatureVar))
	{
		IDiscoFeature dfeature = FDiscoFeatures.take(AFeatureVar);
		emit discoFeatureRemoved(dfeature);
		updateSelfEntityCapabilities();
	}
}

bool ServiceDiscovery::hasDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const
{
	return FDiscoInfo.value(AStreamJid).value(AContactJid).contains(ANode);
}

IDiscoInfo ServiceDiscovery::discoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const
{
	return FDiscoInfo.value(AStreamJid).value(AContactJid).value(ANode);
}

bool ServiceDiscovery::requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	bool sent = false;

	DiscoveryRequest drequest;
	drequest.streamJid = AStreamJid;
	drequest.contactJid = AContactJid;
	drequest.node = ANode;
	if (FInfoRequestsId.values().contains(drequest))
	{
		sent = true;
	}
	else if (FStanzaProcessor && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
		QDomElement query =  iq.addElement("query",NS_DISCO_INFO);
		if (!ANode.isEmpty())
			query.setAttribute("node",ANode);
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,DISCO_TIMEOUT);
		if (sent)
		{
			FInfoRequestsId.insert(iq.id(),drequest);
			//LogDetaile(QString("[ServiceDiscovery] Discovery information request sent to='%1' node='%2' id='%3'").arg(AContactJid.full(),ANode,iq.id()));
		}
		else
		{
			LogError(QString("[ServiceDiscovery] Failed to send discovery information request to='%1' node='%2'").arg(AContactJid.full(),ANode));
		}
	}
	return sent;
}

void ServiceDiscovery::removeDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	if (hasDiscoInfo(AStreamJid,AContactJid,ANode))
	{
		QMap<QString, IDiscoInfo> &dnodeInfo = FDiscoInfo[AStreamJid][AContactJid];
		IDiscoInfo dinfo = dnodeInfo.take(ANode);
		if (dnodeInfo.isEmpty())
			FDiscoInfo[AStreamJid].remove(AContactJid);
		//LogDetaile(QString("[ServiceDiscovery] Discovery information removed jid='%1' node='%2'").arg(AContactJid.full(),ANode));
		emit discoInfoRemoved(dinfo);
	}
}

int ServiceDiscovery::findIdentity(const QList<IDiscoIdentity> &AIdentity, const QString &ACategory, const QString &AType) const
{
	int index = -1;
	for (int i=0; index<0 && i<AIdentity.count(); i++)
		if ((ACategory.isEmpty() || AIdentity.at(i).category == ACategory) && (AType.isEmpty() || AIdentity.at(i).type == AType))
			index = i;
	return index;
}

bool ServiceDiscovery::requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	bool sent = false;

	DiscoveryRequest drequest;
	drequest.streamJid = AStreamJid;
	drequest.contactJid = AContactJid;
	drequest.node = ANode;
	if (FItemsRequestsId.values().contains(drequest))
	{
		sent = true;
	}
	else if (FStanzaProcessor && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
		QDomElement query =  iq.addElement("query",NS_DISCO_ITEMS);
		if (!ANode.isEmpty())
			query.setAttribute("node",ANode);
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,DISCO_TIMEOUT);
		if (sent)
		{
			FItemsRequestsId.insert(iq.id(),drequest);
			LogDetail(QString("[ServiceDiscovery] Discovery items request sent to='%1' node='%2' id='%3'").arg(AContactJid.full(),ANode,iq.id()));
		}
		else
		{
			LogError(QString("[ServiceDiscovery] Failed to send discovery items request to='%1' node='%2'").arg(AContactJid.full(),ANode));
		}
	}
	return sent;
}

void ServiceDiscovery::discoInfoToElem(const IDiscoInfo &AInfo, QDomElement &AElem) const
{
	QDomDocument doc = AElem.ownerDocument();
	foreach(IDiscoIdentity identity, AInfo.identity)
	{
		QDomElement elem = AElem.appendChild(doc.createElement("identity")).toElement();
		elem.setAttribute("category",identity.category);
		elem.setAttribute("type",identity.type);
		if (!identity.name.isEmpty())
			elem.setAttribute("name",identity.name);
		if (!identity.lang.isEmpty())
			elem.setAttribute("xml:lang",identity.lang);

	}
	foreach(QString feature, AInfo.features)
	{
		QDomElement elem = AElem.appendChild(doc.createElement("feature")).toElement();
		elem.setAttribute("var",feature);
	}
	if (FDataForms)
	{
		foreach(IDataForm form, AInfo.extensions)
		{
			FDataForms->xmlForm(form,AElem);
		}
	}
}

void ServiceDiscovery::discoInfoFromElem(const QDomElement &AElem, IDiscoInfo &AInfo) const
{
	AInfo.identity.clear();
	QDomElement elem = AElem.firstChildElement("identity");
	while (!elem.isNull())
	{
		IDiscoIdentity identity;
		identity.category = elem.attribute("category").toLower();
		identity.type = elem.attribute("type").toLower();
		identity.lang = elem.attribute("lang");
		identity.name = elem.attribute("name");
		AInfo.identity.append(identity);
		elem = elem.nextSiblingElement("identity");
	}

	AInfo.features.clear();
	elem = AElem.firstChildElement("feature");
	while (!elem.isNull())
	{
		QString feature = elem.attribute("var");
		if (!feature.isEmpty() && !AInfo.features.contains(feature))
			AInfo.features.append(feature);
		elem = elem.nextSiblingElement("feature");
	}

	if (FDataForms)
	{
		AInfo.extensions.clear();
		elem = AElem.firstChildElement("x");
		while (!elem.isNull())
		{
			if (elem.namespaceURI()==NS_JABBER_DATA)
			{
				IDataForm form = FDataForms->dataForm(elem);
				AInfo.extensions.append(form);
			}
			elem = elem.nextSiblingElement("x");
		}
	}
}

IDiscoInfo ServiceDiscovery::parseDiscoInfo(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const
{
	IDiscoInfo result;
	result.streamJid = ADiscoRequest.streamJid;
	result.contactJid = ADiscoRequest.contactJid;
	result.node = ADiscoRequest.node;

	QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
	if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		result.error.code = err.code();
		result.error.condition = err.condition();
		result.error.message = err.message();
		LogError(QString("[ServiceDiscovery] Failed to request discovery information from '%1' node='%2' id='%3': %4").arg(result.contactJid.full(),result.node,AStanza.id(),err.message()));	
	}
	else if (result.contactJid!=AStanza.from() || result.node!=query.attribute("node"))
	{
		ErrorHandler err(ErrorHandler::NOT_ACCEPTABLE);
		result.error.code = err.code();
		result.error.condition = err.condition();
		result.error.message = err.message();
		LogError(QString("[ServiceDiscovery] Failed to request discovery information from '%1' node='%2' id='%3': %4").arg(result.contactJid.full(),result.node,AStanza.id(),err.message()));	
	}
	else
	{
		discoInfoFromElem(query,result);
		//LogDetaile(QString("[ServiceDiscovery] Discovery information received from '%1' node='%2' id='%3'").arg(result.contactJid.full(),result.node,AStanza.id()));
	}
	return result;
}

IDiscoItems ServiceDiscovery::parseDiscoItems(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const
{
	IDiscoItems result;
	result.streamJid = ADiscoRequest.streamJid;
	result.contactJid = ADiscoRequest.contactJid;
	result.node = ADiscoRequest.node;

	QDomElement query = AStanza.firstElement("query",NS_DISCO_ITEMS);
	if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		result.error.code = err.code();
		result.error.condition = err.condition();
		result.error.message = err.message();
		LogError(QString("[ServiceDiscovery] Failed to request discovery items from '%1' node='%2' id='%3': %4").arg(result.contactJid.full(),result.node,AStanza.id(),err.message()));	
	}
	else if (result.contactJid!=AStanza.from() || result.node!=query.attribute("node"))
	{
		ErrorHandler err(ErrorHandler::NOT_ACCEPTABLE);
		result.error.code = err.code();
		result.error.condition = err.condition();
		result.error.message = err.message();
		LogError(QString("[ServiceDiscovery] Failed to request discovery items from '%1' node='%2' id='%3': %4").arg(result.contactJid.full(),result.node,AStanza.id(),err.message()));	
	}
	else
	{
		QDomElement elem = query.firstChildElement("item");
		while (!elem.isNull())
		{
			IDiscoItem ditem;
			ditem.itemJid = elem.attribute("jid");
			ditem.node = elem.attribute("node");
			ditem.name = elem.attribute("name");
			result.items.append(ditem);
			elem = elem.nextSiblingElement("item");
		}
		LogDetail(QString("[ServiceDiscovery] Discovery items received from '%1' node='%2' id='%3'").arg(result.contactJid.full(),result.node,AStanza.id()));
	}
	return result;
}

void ServiceDiscovery::registerFeatures()
{
	IDiscoFeature dfeature;

	dfeature.var = NS_DISCO;
	dfeature.active = false;
	dfeature.name = tr("Service Discovery");
	dfeature.description = tr("Supports the exchange of the discovery information and items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_INFO;
	dfeature.active = true;
	dfeature.name = tr("Discovery Information");
	dfeature.description = tr("Supports the exchange of the discovery information");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_ITEMS;
	dfeature.active = false;
	dfeature.name = tr("Discovery Items");
	dfeature.description = tr("Supports the exchange of the discovery items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_PUBLISH;
	dfeature.active = false;
	dfeature.name = tr("Publish Items");
	dfeature.description = tr("Supports the publishing of the discovery items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_ENTITY_CAPS;
	dfeature.active = true;
	dfeature.name = tr("Entity Capabilities");
	dfeature.description = tr("Supports the caching of the discovery information");
	insertDiscoFeature(dfeature);

	dfeature.var = "Jid\\20Escaping";
	dfeature.active = true;
	dfeature.name = tr("JID Escaping");
	dfeature.description = tr("Supports the displaying of the jabber identifiers with disallowed characters");
	insertDiscoFeature(dfeature);
}

void ServiceDiscovery::appendQueuedRequest(const QDateTime &ATimeStart, const DiscoveryRequest &ARequest)
{
	QMultiMap<QDateTime,DiscoveryRequest>::const_iterator it = FQueuedRequests.constBegin();
	while (it != FQueuedRequests.constEnd())
	{
		if (it.value().contactJid==ARequest.contactJid && it.value().node==ARequest.node)
			return;
		it++;
	}
	FQueuedRequests.insert(ATimeStart,ARequest);
	if (!FQueueTimer.isActive())
		FQueueTimer.start();
}

void ServiceDiscovery::removeQueuedRequest(const DiscoveryRequest &ARequest)
{
	QMultiMap<QDateTime,DiscoveryRequest>::iterator it = FQueuedRequests.begin();
	while (it!=FQueuedRequests.end())
	{
		if (
		  (ARequest.streamJid.isEmpty() || it.value().streamJid == ARequest.streamJid) &&
		  (ARequest.contactJid.isEmpty() || it.value().contactJid == ARequest.contactJid) &&
		  (ARequest.node.isEmpty() || it.value().node == ARequest.node)
		)
			it = FQueuedRequests.erase(it);
		else
			it++;
	}
}

bool ServiceDiscovery::hasEntityCaps(const EntityCapabilities &ACaps) const
{
	return QFile::exists(capsFileName(ACaps,false)) || QFile::exists(capsFileName(ACaps,true));
}

QString ServiceDiscovery::capsFileName(const EntityCapabilities &ACaps, bool AForJid) const
{
	static bool entered = false;
	static QDir dir(FPluginManager->homePath());
	if (!entered)
	{
		entered = true;
		if (!dir.exists(CAPS_DIRNAME))
			dir.mkdir(CAPS_DIRNAME);
		dir.cd(CAPS_DIRNAME);
	}

	QString hashString = ACaps.hash.isEmpty() ? ACaps.node+ACaps.ver : ACaps.ver+ACaps.hash;
	hashString += AForJid ? ACaps.entityJid.pFull() : "";
	QString fileName = QCryptographicHash::hash(hashString.toUtf8(),QCryptographicHash::Md5).toHex().toLower() + ".xml";
	return dir.absoluteFilePath(fileName);
}

IDiscoInfo ServiceDiscovery::loadEntityCaps(const EntityCapabilities &ACaps) const
{
	QHash<Jid, EntityCapabilities> streamEntityCaps = FEntityCaps.value(ACaps.streamJid);
	QHash<Jid, EntityCapabilities>::const_iterator it = streamEntityCaps.constBegin();
	while (it != streamEntityCaps.constEnd())
	{
		EntityCapabilities caps = it.value();
		if ((!ACaps.hash.isEmpty() || caps.node==ACaps.node) && caps.ver==ACaps.ver && caps.hash==ACaps.hash && hasDiscoInfo(caps.streamJid, caps.entityJid))
		{
			IDiscoInfo dinfo = discoInfo(caps.streamJid,caps.entityJid);
			if (caps.ver == calcCapsHash(dinfo,caps.hash))
				return dinfo;
		}
		it++;
	}

	IDiscoInfo dinfo;
	QString fileName = capsFileName(ACaps,true);
	if (!QFile::exists(fileName))
		fileName = capsFileName(ACaps,false);
	QFile capsFile(fileName);
	if (capsFile.exists() && capsFile.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		doc.setContent(capsFile.readAll(),true);
		capsFile.close();
		QDomElement capsElem = doc.documentElement();
		discoInfoFromElem(capsElem,dinfo);
	}
	return dinfo;
}

bool ServiceDiscovery::saveEntityCaps(const IDiscoInfo &AInfo) const
{
	if (AInfo.error.code==-1 && FEntityCaps.value(AInfo.streamJid).contains(AInfo.contactJid))
	{
		EntityCapabilities caps = FEntityCaps.value(AInfo.streamJid).value(AInfo.contactJid);
		QString capsNode = QString("%1#%2").arg(caps.node).arg(caps.ver);
		if (AInfo.node.isEmpty() || AInfo.node==capsNode)
		{
			if (!hasEntityCaps(caps))
			{
				bool checked = (caps.ver==calcCapsHash(AInfo,caps.hash));
				QDomDocument doc;
				QDomElement capsElem = doc.appendChild(doc.createElement(CAPS_FILE_TAG_NAME)).toElement();
				capsElem.setAttribute("node",caps.node);
				capsElem.setAttribute("ver",caps.ver);
				capsElem.setAttribute("hash",caps.hash);
				if (!checked)
					capsElem.setAttribute("jid",caps.entityJid.pFull());
				discoInfoToElem(AInfo,capsElem);
				QFile capsFile(capsFileName(caps,!checked));
				if (capsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
				{
					capsFile.write(doc.toByteArray());
					capsFile.close();
				}
			}
			return true;
		}
	}
	return false;
}

QString ServiceDiscovery::calcCapsHash(const IDiscoInfo &AInfo, const QString &AHash) const
{
	if (AHash==CAPS_HASH_SHA1 || AHash==CAPS_HASH_MD5)
	{
		QStringList hashList;
		QStringList sortList;

		foreach(IDiscoIdentity identity, AInfo.identity)
			sortList.append(identity.category+"/"+identity.type+"/"+identity.lang+"/"+identity.name);
		qSort(sortList);
		hashList += sortList;

		sortList = AInfo.features;
		qSort(sortList);
		hashList += sortList;

		if (FDataForms && !AInfo.extensions.isEmpty())
		{
			QMultiMap<QString, int> sortForms;
			for (int index=0; index<AInfo.extensions.count();index++)
				sortForms.insertMulti(FDataForms->fieldValue("FORM_TYPE",AInfo.extensions.at(index).fields).toString(),index);

			QMultiMap<QString, int>::const_iterator iforms = sortForms.constBegin();
			while (iforms != sortForms.constEnd())
			{
				hashList += iforms.key();
				QMultiMap<QString,QStringList> sortFields;
				foreach(IDataField field, AInfo.extensions.at(iforms.value()).fields)
				{
					if (field.var != "FORM_TYPE")
					{
						QStringList values;
						if (field.value.type() == QVariant::StringList)
							values = field.value.toStringList();
						else if (field.value.type() == QVariant::Bool)
							values +=(field.value.toBool() ? "1" : "0");
						else
							values += field.value.toString();
						qSort(values);
						sortFields.insertMulti(field.var,values);
					}
				}
				QMultiMap<QString,QStringList>::const_iterator ifields = sortFields.constBegin();
				while (ifields != sortFields.constEnd())
				{
					hashList += ifields.key();
					hashList += ifields.value();
					ifields++;
				}
				iforms++;
			}
		}
		hashList.append("");
		QByteArray hashData = hashList.join("<").toUtf8();
		return QCryptographicHash::hash(hashData, AHash==CAPS_HASH_SHA1 ? QCryptographicHash::Sha1 : QCryptographicHash::Md5).toBase64();
	}
	return QString::null;
}

bool ServiceDiscovery::compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const
{
	foreach(IDiscoIdentity identity,AIdentities)
	{
		if (
			(AWith.category.isEmpty() || AWith.category==identity.category) &&
			(AWith.type.isEmpty() || AWith.type==identity.type) &&
			(AWith.lang.isEmpty() || AWith.lang==identity.lang) &&
			(AWith.name.isEmpty() || AWith.name==identity.name)
			)
			return true;
	}
	return false;
}

bool ServiceDiscovery::compareFeatures(const QStringList &AFeatures, const QStringList &AWith) const
{
	if (!AWith.isEmpty())
		foreach(QString feature, AWith)
			if (!AFeatures.contains(feature))
				return false;
	return true;
}

void ServiceDiscovery::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.conditions.append(SHC_DISCO_INFO);
		FSHIInfo.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.conditions.clear();
		shandle.conditions.append(SHC_DISCO_ITEMS);
		FSHIItems.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.conditions.clear();
		shandle.conditions.append(SHC_PRESENCE);
		shandle.direction = IStanzaHandle::DirectionOut;
		FSHIPresenceOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.order = SHO_PI_SERVICEDISCOVERY;
		shandle.direction = IStanzaHandle::DirectionIn;
		FSHIPresenceIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	EntityCapabilities &myCaps = FSelfCaps[AXmppStream->streamJid()];
	myCaps.streamJid = AXmppStream->streamJid();
	myCaps.entityJid = AXmppStream->streamJid();
	myCaps.node = CLIENT_HOME_PAGE;
	myCaps.hash = CAPS_HASH_SHA1;
	myCaps.ver = calcCapsHash(selfDiscoInfo(myCaps.entityJid),myCaps.hash);

	requestDiscoInfo(AXmppStream->streamJid(),AXmppStream->streamJid().domain());
}

void ServiceDiscovery::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIInfo.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIItems.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceOut.take(AXmppStream->streamJid()));
	}

	DiscoveryRequest request;
	request.streamJid = AXmppStream->streamJid();
	removeQueuedRequest(request);

	foreach(Jid contactJid, FDiscoInfo.value(AXmppStream->streamJid()).keys()) {
		foreach(QString node, FDiscoInfo.value(AXmppStream->streamJid()).value(contactJid).keys()) {
			removeDiscoInfo(AXmppStream->streamJid(),contactJid,node);
		};
	};

	FSelfCaps.remove(AXmppStream->streamJid());
	FEntityCaps.remove(AXmppStream->streamJid());
	FDiscoInfo.remove(AXmppStream->streamJid());
}

void ServiceDiscovery::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
	{
		if (!AItem.itemJid.node().isEmpty())
		{
			DiscoveryRequest request;
			request.streamJid = APresence->streamJid();
			request.contactJid = AItem.itemJid;
			removeQueuedRequest(request);
			removeDiscoInfo(APresence->streamJid(),AItem.itemJid);
		}
		FEntityCaps[APresence->streamJid()].remove(AItem.itemJid);
	}
}

void ServiceDiscovery::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.subscription!=SUBSCRIPTION_REMOVE && AItem.itemJid.node().isEmpty() && ARoster->isOpen() && !hasDiscoInfo(ARoster->streamJid(), AItem.itemJid))
	{
		DiscoveryRequest request;
		request.streamJid = ARoster->streamJid();
		request.contactJid = AItem.itemJid;
		appendQueuedRequest(QUEUE_REQUEST_START,request);
	}
}

void ServiceDiscovery::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	DiscoveryRequest request;
	request.streamJid = ADiscoInfo.streamJid;
	request.contactJid = ADiscoInfo.contactJid;
	request.node = ADiscoInfo.node;
	removeQueuedRequest(request);
}

void ServiceDiscovery::onQueueTimerTimeout()
{
	bool sended = false;
	QMultiMap<QDateTime,DiscoveryRequest>::iterator it = FQueuedRequests.begin();
	while (!sended && it!=FQueuedRequests.end() && it.key()<QDateTime::currentDateTime())
	{
		DiscoveryRequest request = it.value();
		sended = requestDiscoInfo(request.streamJid,request.contactJid,request.node);
		it = FQueuedRequests.erase(it);
	}

	if (FQueuedRequests.isEmpty())
	{
		FQueueTimer.stop();
	}
}

void ServiceDiscovery::onSelfCapsChanged()
{
	foreach(Jid streamJid, FSelfCaps.keys())
	{
		EntityCapabilities &myCaps = FSelfCaps[streamJid];
		QString newVer = calcCapsHash(selfDiscoInfo(streamJid),myCaps.hash);
		if (myCaps.ver != newVer)
		{
			myCaps.ver = newVer;
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			if (presence && presence->isOpen())
			{
				LogDetail(QString("[ServiceDiscovery] Updating self entity caps on stream '%1'").arg(presence->streamJid().bare()));
				presence->setPresence(presence->show(),presence->status(),presence->priority());
			}
		}
	}
	FUpdateSelfCapsStarted = false;
}

Q_EXPORT_PLUGIN2(plg_servicediscovery, ServiceDiscovery)
