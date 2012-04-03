#include "sipmanager.h"

SipManager::SipManager(QObject *parent) :
	QObject(parent)
{
}

QObject *SipManager::instance()
{
	return this;
}

QUuid SipManager::pluginUuid() const
{
	return SIPMANAGER_UUID;
}

void SipManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SIP Manager");
	APluginInfo->description = tr("Allows to make voice and video calls over SIP protocol");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Gorshkov V.A.";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SipManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		if(FMetaContacts)
		{
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowCreated(IMetaTabWindow*)), SLOT(onMetaTabWindowCreated(IMetaTabWindow*)));
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowDestroyed(IMetaTabWindow*)), SLOT(onMetaTabWindowDestroyed(IMetaTabWindow*)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{

		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
			connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

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
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onXmppStreamOpened(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onXmppStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onXmppStreamClosed(IXmppStream *)));
	}

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	connect(this, SIGNAL(streamCreated(const QString&)), this, SLOT(onStreamCreated(const QString&)));

	return FStanzaProcessor!=NULL;
}

bool SipManager::initObjects()
{
	// TODO: implementation
	return true;
}

bool SipManager::initSettings()
{
	// TODO: implementation
	return true;
}

bool SipManager::startPlugin()
{
	// TODO: implementation
	return true;
}

bool SipManager::isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	// TODO: implementation
	return true;
}

ISipCall *SipManager::newCall()
{
	// TODO: implementation
	return NULL;
}

QList<ISipCall*> SipManager::findCalls(const Jid &AStreamJid)
{
	// TODO: implementation
	return QList<ISipCall*>();
}

QList<ISipDevice> SipManager::availDevices(ISipDevice::Type AType) const
{
	// TODO: implementation
	return QList<ISipDevice>();
}

ISipDevice SipManager::getDevice(ISipDevice::Type AType, int ADeviceId) const
{
	// TODO: implementation
	return ISipDevice();
}

void SipManager::insertSipCallHandler(int AOrder, ISipCallHandler *AHandler)
{
	// TODO: implementation
}

void SipManager::removeSipCallHandler(int AOrder, ISipCallHandler *AHandler)
{
	// TODO: implementation
}
