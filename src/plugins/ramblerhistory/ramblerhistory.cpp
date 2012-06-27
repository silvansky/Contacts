#include "ramblerhistory.h"

#define ARCHIVE_TIMEOUT       30000
#define SHC_PREFS_UPDATE      "/iq[@type='set']/pref[@xmlns=" NS_RAMBLER_ARCHIVE "]"

RamblerHistory::RamblerHistory()
{
	FGateways = NULL;
	FDiscovery = NULL;
	FXmppStreams = NULL;
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
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RamblerHistory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

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

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
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
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

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
	return true;
}

QMultiMap<int, IOptionsWidget *> RamblerHistory::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	Q_UNUSED(AParent);
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FXmppStreams && ANodeId == OPN_COMMON)
	{
		foreach(IXmppStream *xmppStream, FXmppStreams->xmppStreams())
		{
			if (FXmppStreams->isActive(xmppStream))
			{
				widgets.insertMulti(OWO_COMMON_SINC_HISTORY,new HistoryOptionsWidget(this,xmppStream,AParent));
				break;
			}
		}
	}
	return widgets;
}

bool RamblerHistory::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIPrefsUpdate.value(AStreamJid)==AHandlerId && AStanza.isFromServer())
	{
		QDomElement prefElem = AStanza.firstElement("pref",NS_RAMBLER_ARCHIVE);
		applyArchivePrefs(AStreamJid,prefElem);

		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	return false;
}

void RamblerHistory::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FRetrieveRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			IHistoryMessages result;
			QDomElement chatElem = AStanza.firstElement("chat",NS_RAMBLER_ARCHIVE);
			result.with = chatElem.attribute("with");

			QDomElement elem = chatElem.firstChildElement();
			while (!elem.isNull())
			{
				if (elem.tagName() == "to" || elem.tagName() == "from")
				{
					Message message;
					if (elem.tagName() == "to")
					{
						message.setTo(result.with.full());
						message.setFrom(AStreamJid.pBare());
					}
					else
					{
						message.setTo(AStreamJid.pBare());
						message.setFrom(result.with.full());
					}

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

			LogDetail(QString("[RamblerHistory] Loaded %1 history messages with '%2', id='%3'").arg(result.messages.count()).arg(result.with.full(),AStanza.id()));
			emit serverMessagesLoaded(AStanza.id(), result);
		}
		FRetrieveRequests.remove(AStanza.id());
	}
	else if (FPrefsLoadRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QDomElement prefElem = AStanza.firstElement("pref",NS_RAMBLER_ARCHIVE);
			applyArchivePrefs(AStreamJid,prefElem);
		}
		FPrefsLoadRequests.remove(AStanza.id());
	}
	else if (FPrefsSaveRequests.contains(AStanza.id()))
	{
		FPrefsSaveRequests.remove(AStanza.id());
	}

	if (AStanza.type() == "result")
	{
		LogDetail(QString("[RamblerHistory] Request id='%1' to '%2' precessed successfully").arg(AStanza.id(),AStreamJid.full()));
		emit requestCompleted(AStanza.id());
	}
	else
	{
		ErrorHandler err(AStanza.element());
		LogError(QString("[RamblerHistory] Failed to process request id='%1' to '%2': %3").arg(AStanza.id(),AStreamJid.full(),err.message()));
		emit requestFailed(AStanza.id(),ErrorHandler(AStanza.element()).message());
	}
}

bool RamblerHistory::isReady(const Jid &AStreamJid) const
{
	return !FHistoryPrefs.value(AStreamJid).autoSave.isEmpty();
}

bool RamblerHistory::isSupported(const Jid &AStreamJid) const
{
	return FDiscovery==NULL || !FDiscovery->hasDiscoInfo(AStreamJid, AStreamJid.domain()) || FDiscovery->discoInfo(AStreamJid,AStreamJid.domain()).features.contains(NS_RAMBLER_ARCHIVE);
}

IHistoryStreamPrefs RamblerHistory::historyPrefs(const Jid &AStreamJid) const
{
	return FHistoryPrefs.value(AStreamJid);
}

IHistoryItemPrefs RamblerHistory::historyItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const
{
	IHistoryItemPrefs domainPrefs, barePrefs, fullPrefs;
	IHistoryStreamPrefs prefs = historyPrefs(AStreamJid);
	for (QHash<Jid, IHistoryItemPrefs>::const_iterator it = prefs.itemPrefs.constBegin(); it != prefs.itemPrefs.constEnd(); it++)
	{
		QString node = it.key().pNode();
		QString domain = it.key().pDomain();
		QString resource = it.key().pResource();
		if (domain == AItemJid.pDomain())
		{
			if (node == AItemJid.pNode())
			{
				if (resource == AItemJid.pResource())
				{
					fullPrefs = it.value();
					break;
				}
				else if (resource.isEmpty())
				{
					barePrefs = it.value();
				}
			}
			else if (resource.isEmpty() && node.isEmpty())
			{
				domainPrefs = it.value();
			}
		}
	}

	if (!fullPrefs.save.isEmpty())
		return fullPrefs;
	else if (!barePrefs.save.isEmpty())
		return barePrefs;
	else if (!domainPrefs.save.isEmpty())
		return domainPrefs;

	return prefs.defaultPrefs;
}

QString RamblerHistory::setHistoryPrefs(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs)
{
	if (isReady(AStreamJid))
	{
		Stanza save("iq");
		save.setType("set").setId(FStanzaProcessor->newId());
		QDomElement prefElem = save.addElement("pref",NS_RAMBLER_ARCHIVE);

		IHistoryStreamPrefs oldPrefs = historyPrefs(AStreamJid);
		IHistoryStreamPrefs newPrefs = oldPrefs;

		bool autoChanged = newPrefs.autoSave != APrefs.autoSave;
		if (autoChanged)
		{
			QDomElement autoElem = prefElem.appendChild(save.createElement("auto")).toElement();
			autoElem.setAttribute("save",APrefs.autoSave);
		}

		bool defChanged = oldPrefs.defaultPrefs.save != newPrefs.defaultPrefs.save;
		if (defChanged)
		{
			QDomElement defElem = prefElem.appendChild(save.createElement("default")).toElement();
			defElem.setAttribute("save",newPrefs.defaultPrefs.save);
		}

		bool itemsChanged = false;
		foreach(Jid itemJid, newPrefs.itemPrefs.keys())
		{
			IHistoryItemPrefs newItemPrefs = newPrefs.itemPrefs.value(itemJid);
			IHistoryItemPrefs oldItemPrefs = oldPrefs.itemPrefs.value(itemJid);
			bool itemChanged = oldItemPrefs.save != newItemPrefs.save;
			if (itemChanged)
			{
				if (!newItemPrefs.save.isEmpty())
				{
					QDomElement itemElem = prefElem.appendChild(save.createElement("item")).toElement();
					itemElem.setAttribute("jid",itemJid.full());
					itemElem.setAttribute("save",newItemPrefs.save);
				}
				else
				{
					QDomElement itemElem = prefElem.appendChild(save.createElement("itemremove")).toElement();
					itemElem.setAttribute("jid",itemJid.full());
				}
			}
			itemsChanged |= itemChanged;
		}

		if (autoChanged || defChanged || itemsChanged)
		{
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,save,ARCHIVE_TIMEOUT))
			{
				FPrefsSaveRequests.insert(save.id(),AStreamJid);
				LogDetail(QString("[RamblerHistory] Save history preferences request sent to '%1', id='%2'").arg(AStreamJid.full(),save.id()));
				return save.id();
			}
			else
			{
				LogError(QString("[RamblerHistory] Failed to send preferences save request to '%1'").arg(AStreamJid.full()));
			}
		}
	}
	return QString::null;
}

QString RamblerHistory::loadServerMessages(const Jid &AStreamJid, const IHistoryRetrieve &ARetrieve)
{
	if (FStanzaProcessor)
	{
		Stanza retrieve("iq");
		retrieve.setType("get").setId(FStanzaProcessor->newId());
		QDomElement retrieveElem = retrieve.addElement("retrieve",NS_RAMBLER_ARCHIVE);
		retrieveElem.setAttribute("with",ARetrieve.with.full());
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
			LogDetail(QString("[RamblerHistory] Load history with '%1' request sent to '%2', id='%3'").arg(ARetrieve.with.full(),AStreamJid.full(),retrieve.id()));
			FRetrieveRequests.insert(retrieve.id(),ARetrieve.with);
			return retrieve.id();
		}
		else
		{
			LogError(QString("[RamblerHistory] Failed to send load history request with '%1' request to '%2'").arg(ARetrieve.with.full(),AStreamJid.full()));
		}
	}
	return QString::null;
}

QWidget *RamblerHistory::showViewHistoryWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	ViewHistoryWindow *viewWindow = NULL;
	if (isSupported(AStreamJid))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster)
		{
			viewWindow = findViewWindow(roster,AContactJid);
			if (!viewWindow)
			{
				viewWindow = new ViewHistoryWindow(roster,FGateways,AContactJid);
				connect(viewWindow,SIGNAL(windowDestroyed()),SLOT(onViewHistoryWindowDestroyed()));
				FViewWindows.insertMulti(roster,viewWindow);
			}
			WidgetManager::showActivateRaiseWindow(viewWindow->window());
			WidgetManager::alignWindow(viewWindow->window(),Qt::AlignCenter);
		}
	}
	return viewWindow;
}

QString RamblerHistory::loadServerPrefs(const Jid &AStreamJid)
{
	Stanza load("iq");
	load.setType("get").setId(FStanzaProcessor!=NULL ? FStanzaProcessor->newId() : QString::null);
	load.addElement("pref",NS_RAMBLER_ARCHIVE);
	if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,load,ARCHIVE_TIMEOUT))
	{
		FPrefsLoadRequests.insert(load.id(),AStreamJid);
		LogDetail(QString("[RamblerHistory] Load history preferences request sent to '%1', id='%2'").arg(AStreamJid.full(),load.id()));
		return load.id();
	}
	else
	{
		LogError(QString("[RamblerHistory] Failed to send preferences load request to '%1'").arg(AStreamJid.full()));
	}
	return QString::null;
}

void RamblerHistory::applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem)
{
	if (!AElem.isNull())
	{
		bool initPrefs = !isReady(AStreamJid);
		IHistoryStreamPrefs &prefs = FHistoryPrefs[AStreamJid];

		QDomElement autoElem = AElem.firstChildElement("auto");
		if (!autoElem.isNull())
			prefs.autoSave = autoElem.attribute("save",HISTORY_SAVE_FALSE);
		else if (initPrefs)
			prefs.autoSave = HISTORY_SAVE_FALSE;

		QDomElement defElem = AElem.firstChildElement("default");
		if (!defElem.isNull())
		{
			prefs.defaultPrefs.save = defElem.attribute("save",HISTORY_SAVE_FALSE);
		}
		else if (initPrefs)
		{
			prefs.defaultPrefs.save = HISTORY_SAVE_FALSE;
		}

		QDomElement itemElem = AElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			IHistoryItemPrefs itemPrefs;
			Jid itemJid = itemElem.attribute("jid");
			if (itemElem.hasAttribute("save"))
			{
				itemPrefs.save = itemElem.attribute("save",HISTORY_SAVE_FALSE);
				prefs.itemPrefs.insert(itemJid,itemPrefs);
			}
			else
			{
				itemPrefs.save = QString::null;
				prefs.itemPrefs.remove(itemJid);
			}
			emit historyItemPrefsChanged(AStreamJid,itemJid,itemPrefs);
			itemElem = itemElem.nextSiblingElement("item");
		}

		QDomElement removeElem = AElem.firstChildElement("itemremove");
		while (!removeElem.isNull())
		{
			Jid itemJid = itemElem.attribute("jid");
			prefs.itemPrefs.remove(itemJid);
			emit historyItemPrefsChanged(AStreamJid,itemJid,IHistoryItemPrefs());
			removeElem = itemElem.nextSiblingElement("itemremove");
		}

		emit historyPrefsChanged(AStreamJid,prefs);
	}
}

ViewHistoryWindow *RamblerHistory::findViewWindow(IRoster *ARoster, const Jid &AContactJid) const
{
	foreach(ViewHistoryWindow *window, FViewWindows.values(ARoster))
		if (AContactJid && window->contactJid())
			return window;
	return NULL;
}

void RamblerHistory::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_PREFS_UPDATE);
		FSHIPrefsUpdate.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
}

void RamblerHistory::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPrefsUpdate.take(AXmppStream->streamJid()));
	}

	FHistoryPrefs.remove(AXmppStream->streamJid());

	emit historyPrefsChanged(AXmppStream->streamJid(),historyPrefs(AXmppStream->streamJid()));
}

void RamblerHistory::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (!FHistoryPrefs.contains(AInfo.streamJid) && AInfo.node.isEmpty() && AInfo.streamJid.pDomain()==AInfo.contactJid.pFull())
	{
		FHistoryPrefs[AInfo.streamJid] = IHistoryStreamPrefs();
		if (AInfo.features.contains(NS_RAMBLER_ARCHIVE))
			loadServerPrefs(AInfo.streamJid);
	}
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
