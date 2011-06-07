#include "ramblerhistory.h"
#include <utils/log.h>

#define ARCHIVE_TIMEOUT 30000

RamblerHistory::RamblerHistory()
{
	FDiscovery = NULL;
	FRosterPlugin = NULL;
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
	APluginInfo->homePage = "http://friends.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RamblerHistory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

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
	Q_UNUSED(AParent);
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_COMMON)
	{
		//widgets.insertMulti(OWO_COMMON_SINC_HISTORY,FOptionsManager->optionsNodeWidget(Options::node(OPV_MISC_HISTORY_SAVE_ON_SERVER),tr("Store the history of communication in my Rambler.Pochta"),AParent));
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
				if (elem.tagName() == "to" || elem.tagName() == "from")
				{
					Message message;
					if (elem.tagName() == "to")
						message.setTo(result.with.eFull());
					else
						message.setFrom(result.with.eFull());

					message.setType(Message::Chat);
					message.setDateTime(DateTime(elem.attribute("ctime")).toLocal());
					message.setBody(elem.firstChildElement("body").text());

					result.messages.append(message);
				}
				elem = elem.nextSiblingElement();
			}

			elem = chatElem.firstChildElement("first");
			while (!elem.isNull() && elem.namespaceURI()!=NS_RAMBLER_ARCHIVE_RSM)
				elem = elem.nextSiblingElement("first");
			result.beforeId = elem.firstChildElement("id").text();
			result.beforeTime = DateTime(elem.firstChildElement("ctime").text()).toLocal();

			emit serverMessagesLoaded(AStanza.id(), result);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			Log(QString("[Rambler history stanza error] %1 : %2").arg(AStanza.id(), err.message()));
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
		Log(QString("[Rambler history request timeout] %1 : %2").arg(AStanzaId, err.message()));
		emit requestFailed(AStanzaId, err.message());
	}
}

bool RamblerHistory::isSupported(const Jid &AStreamJid) const
{
	return FDiscovery==NULL || !FDiscovery->hasDiscoInfo(AStreamJid, AStreamJid.domain()) || FDiscovery->discoInfo(AStreamJid,AStreamJid.domain()).features.contains(NS_RAMBLER_ARCHIVE);
}

QString RamblerHistory::loadServerMessages(const Jid &AStreamJid, const IRamblerHistoryRetrieve &ARetrieve)
{
	if (FStanzaProcessor)
	{
		Stanza retrieve("iq");
		retrieve.setType("get").setId(FStanzaProcessor->newId());
		QDomElement retrieveElem = retrieve.addElement("retrieve",NS_RAMBLER_ARCHIVE);
		retrieveElem.setAttribute("with",ARetrieve.with.eFull());
		retrieveElem.setAttribute("last",ARetrieve.count);
		if (!ARetrieve.beforeId.isEmpty() || !ARetrieve.beforeTime.isNull())
		{
			QDomElement before = retrieveElem.appendChild(retrieve.createElement("before")).toElement();
			if (!ARetrieve.beforeId.isEmpty())
				before.setAttribute("id",ARetrieve.beforeId);
			if (!ARetrieve.beforeTime.isNull())
				before.setAttribute("ctime",DateTime(ARetrieve.beforeTime).toX85DateTime(true));
		}
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,retrieve,ARCHIVE_TIMEOUT))
		{
			FRetrieveRequests.append(retrieve.id());
			return retrieve.id();
		}
	}
	return QString::null;
}

QWidget *RamblerHistory::showViewHistoryWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	ViewHistoryWindow *window = NULL;
	if (isSupported(AStreamJid))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster)
		{
			window = findViewWindow(roster,AContactJid);
			if (!window)
			{
				window = new ViewHistoryWindow(roster,AContactJid);
				connect(window,SIGNAL(windowDestroyed()),SLOT(onViewHistoryWindowDestroyed()));
				FViewWindows.insertMulti(roster,window);
			}
			WidgetManager::showActivateRaiseWindow(window->parentWidget()!=NULL ? window->parentWidget() : window);
		}
	}
	return window;
}


ViewHistoryWindow *RamblerHistory::findViewWindow(IRoster *ARoster, const Jid &AContactJid) const
{
	foreach(ViewHistoryWindow *window, FViewWindows.values(ARoster))
		if (AContactJid && window->contactJid())
			return window;
	return NULL;
}

void RamblerHistory::onRosterRemoved(IRoster *ARoster)
{
	foreach(ViewHistoryWindow *window, FViewWindows.values(ARoster))
		delete window;
	FViewWindows.remove(ARoster);
}

void RamblerHistory::onViewHistoryWindowDestroyed()
{
	ViewHistoryWindow *window = qobject_cast<ViewHistoryWindow *>(sender());
	IRoster *roster = FViewWindows.key(window);
	FViewWindows.remove(roster,window);
}

Q_EXPORT_PLUGIN2(plg_ramblerhistory, RamblerHistory)
