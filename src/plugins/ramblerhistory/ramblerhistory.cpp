#include "ramblerhistory.h"

#define ARCHIVE_TIMEOUT 30000

RamblerHistory::RamblerHistory()
{
	FOptionsManager = NULL;
	FStanzaProcessor = NULL;
}

RamblerHistory::~RamblerHistory()
{

}

void RamblerHistory::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Rambler History");
	APluginInfo->description = tr("Allows other modules to get access to message history on rambler server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RamblerHistory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		
	return FStanzaProcessor!=NULL;
}

bool RamblerHistory::initObjects()
{
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

bool RamblerHistory::initSettings()
{
	Options::setDefaultValue(OPV_MISC_HISTORY_SAVE_ON_SERVER, true);
	return true;
}

QMultiMap<int, IOptionsWidget *> RamblerHistory::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_COMMON)
	{
		widgets.insertMulti(OWO_COMMON_SINC_HISTORY,FOptionsManager->optionsNodeWidget(Options::node(OPV_MISC_HISTORY_SAVE_ON_SERVER),tr("Store the history of communication in my Rambler.Pochta"),AParent));
	}
	return widgets;
}

void RamblerHistory::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FRetrieveRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			IRamblerHistoryMessages result;
			QDomElement chatElem = AStanza.firstElement("chat",NS_RAMBLER_ARCHIVE);
			result.with = chatElem.attribute("with");

			QDomElement elem = chatElem.firstChildElement();
			while (!elem.isNull())
			{
				Message message;
				message.setType(Message::Chat);
				message.setDateTime(DateTime(elem.attribute("utc")).toLocal());

				if (elem.tagName() == "to")
					message.setTo(result.with.eFull());
				else
					message.setFrom(result.with.eFull());

				message.setBody(elem.firstChildElement("body").text());
				result.messages.append(message);

				elem = elem.nextSiblingElement();
			}
			emit serverMessagesLoaded(AStanza.id(), result);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			emit requestFailed(AStanza.id(), err.message());
		}
		FRetrieveRequests.removeAll(AStanza.id());
	}
}

void RamblerHistory::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FRetrieveRequests.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		emit requestFailed(AStanzaId, err.message());
	}
}

QString RamblerHistory::loadServerMessages(const Jid &AStreamJid, const IRamblerHistoryRetrieve &ARetrieve)
{
	if (FStanzaProcessor)
	{
		Stanza retrieve("iq");
		retrieve.setType("get").setId(FStanzaProcessor->newId());
		QDomElement retrieveElem = retrieve.addElement("retrieve",NS_RAMBLER_ARCHIVE);
		retrieveElem.setAttribute("with",ARetrieve.with.eFull());
		if (ARetrieve.count > 0)
			retrieveElem.setAttribute("last",ARetrieve.count);
		if(!ARetrieve.before.isNull())
			retrieveElem.setAttribute("before",DateTime(ARetrieve.before).toX85DateTime());
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,retrieve,ARCHIVE_TIMEOUT))
		{
			FRetrieveRequests.append(retrieve.id());
			return retrieve.id();
		}
	}
	return QString::null;
}

Q_EXPORT_PLUGIN2(plg_ramblerhistory, RamblerHistory)
