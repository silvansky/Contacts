#include "connectionmanager.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>

ConnectionManager::ConnectionManager()
{
	FAccountManager = NULL;
	FOptionsManager = NULL;
}

ConnectionManager::~ConnectionManager()
{

}

void ConnectionManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Connection Manager");
	APluginInfo->description = tr("Allows to use different types of connections to a Jabber server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool ConnectionManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	QList<IPlugin *> plugins = APluginManager->pluginInterface("IConnectionPlugin");
	foreach (IPlugin *plugin, plugins)
	{
		IConnectionPlugin *cplugin = qobject_cast<IConnectionPlugin *>(plugin->instance());
		if (cplugin)
		{
			FPlugins.insert(cplugin->pluginId(), cplugin);
			connect(cplugin->instance(),SIGNAL(connectionCreated(IConnection *)),SIGNAL(connectionCreated(IConnection *)));
			connect(cplugin->instance(),SIGNAL(connectionDestroyed(IConnection *)),SIGNAL(connectionDestroyed(IConnection *)));
		}
	}

	IPlugin *plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(changed(IAccount *, const OptionsNode &)),
				SLOT(onAccountOptionsChanged(IAccount *, const OptionsNode &)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));

	return !FPlugins.isEmpty();
}

bool ConnectionManager::initObjects()
{
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_TYPE,QString("DefaultConnection"));

	Options::setDefaultValue(OPV_PROXY_DEFAULT,QString(APPLICATION_PROXY_REF_UUID));
	Options::setDefaultValue(OPV_PROXY_NAME,tr("New Proxy"));
	Options::setDefaultValue(OPV_PROXY_TYPE,(int)QNetworkProxy::NoProxy);
	Options::setDefaultValue(OPV_PROXY_PORT,1080);

	return true;
}

bool ConnectionManager::initSettings()
{
	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_CONNECTION, OPN_CONNECTION, tr("Connection"), MNI_CONNECTION_OPTIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> ConnectionManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_CONNECTION)
	{
		IAccount *account = FAccountManager->accounts().value(0);
		OptionsNode cnode = account!=NULL ? account->optionsNode().node("connection", account->optionsNode().node("connection-type").value().toString()) : OptionsNode();
		widgets.insertMulti(OWO_CONNECTION-1, FOptionsManager->optionsHeaderWidget(QString::null,tr("Internet connection"),AParent));
		widgets.insertMulti(OWO_CONNECTION, new ProxyOptionsWidget(this, cnode, AParent));
	}
	return widgets;
}

QList<QString> ConnectionManager::pluginList() const
{
	return FPlugins.keys();
}

IConnectionPlugin *ConnectionManager::pluginById(const QString &APluginId) const
{
	return FPlugins.value(APluginId,NULL);
}

QList<QUuid> ConnectionManager::proxyList() const
{
	QList<QUuid> plist;
	foreach(QString proxyId, Options::node(OPV_PROXY_ROOT).childNSpaces("proxy")) {
		plist.append(proxyId); }
	return plist;
}

IConnectionProxy ConnectionManager::proxyById(const QUuid &AProxyId) const
{
	static const IConnectionProxy noProxy = {" "+tr("<No Proxy>"), QNetworkProxy(QNetworkProxy::NoProxy) };

	if (!AProxyId.isNull())
	{
		OptionsNode pnode;
		QList<QUuid> plist = proxyList();
		if (plist.contains(AProxyId))
			pnode = Options::node(OPV_PROXY_ITEM,AProxyId.toString());
		else if (plist.contains(defaultProxy()))
			pnode = Options::node(OPV_PROXY_ITEM,defaultProxy().toString());

		if (!pnode.isNull())
		{
			IConnectionProxy proxy;
			proxy.name = pnode.value("name").toString();
			proxy.proxy.setType((QNetworkProxy::ProxyType)pnode.value("type").toInt());
			proxy.proxy.setHostName(pnode.value("host").toString());
			proxy.proxy.setPort(pnode.value("port").toInt());
			proxy.proxy.setUser(pnode.value("user").toString());
			proxy.proxy.setPassword(Options::decrypt(pnode.value("pass").toByteArray()).toString());
			return proxy;
		}
	}
	return noProxy;
}

void ConnectionManager::setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy)
{
	if (!AProxyId.isNull() && AProxyId!=APPLICATION_PROXY_REF_UUID)
	{
		OptionsNode pnode = Options::node(OPV_PROXY_ITEM,AProxyId.toString());
		pnode.setValue(AProxy.name,"name");
		pnode.setValue(AProxy.proxy.type(),"type");
		pnode.setValue(AProxy.proxy.hostName(),"host");
		pnode.setValue(AProxy.proxy.port(),"port");
		pnode.setValue(AProxy.proxy.user(),"user");
		pnode.setValue(Options::encrypt(AProxy.proxy.password()),"pass");
		emit proxyChanged(AProxyId, AProxy);
	}
}

void ConnectionManager::removeProxy(const QUuid &AProxyId)
{
	if (proxyList().contains(AProxyId))
	{
		if (defaultProxy() == AProxyId)
			setDefaultProxy(QUuid());
		Options::node(OPV_PROXY_ROOT).removeChilds("proxy",AProxyId.toString());
		emit proxyRemoved(AProxyId);
	}
}

QUuid ConnectionManager::defaultProxy() const
{
	return Options::node(OPV_PROXY_DEFAULT).value().toString();
}

void ConnectionManager::setDefaultProxy(const QUuid &AProxyId)
{
	if (defaultProxy()!=AProxyId && (AProxyId.isNull() || proxyList().contains(AProxyId)))
	{
		Options::node(OPV_PROXY_DEFAULT).setValue(AProxyId.toString());
		QNetworkProxy::setApplicationProxy(proxyById(AProxyId).proxy);
		emit defaultProxyChanged(AProxyId);
	}
}

QUuid ConnectionManager::loadProxySettings(const OptionsNode &ANode) const
{
	return ANode.value().toString();
}

IConnection *ConnectionManager::updateAccountConnection(IAccount *AAccount) const
{
	if (AAccount->isActive())
	{
		OptionsNode aoptions = AAccount->optionsNode();
		QString pluginId = aoptions.value("connection-type").toString();
		IConnectionPlugin *plugin = FPlugins.contains(pluginId) ? FPlugins.value(pluginId) : FPlugins.values().value(0);
		IConnection *connection = AAccount->xmppStream()->connection();
		if (connection && connection->ownerPlugin()!=plugin)
		{
			AAccount->xmppStream()->setConnection(NULL);
			delete connection->instance();
			connection = NULL;
		}
		if (plugin!=NULL && connection==NULL)
		{
			connection = plugin->newConnection(aoptions.node("connection",pluginId),AAccount->xmppStream()->instance());
			AAccount->xmppStream()->setConnection(connection);
			LogDetaile(QString("[ConnectionManager] Inserted IConnection from plugin '%1' to XMPP stream '%2'").arg(plugin->pluginName(),AAccount->streamJid().full()));
		}
		return connection;
	}
	return NULL;
}

void ConnectionManager::onAccountShown(IAccount *AAccount)
{
	updateAccountConnection(AAccount);
}

void ConnectionManager::onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode)
{
	const OptionsNode &aoptions = AAccount->optionsNode();
	if (aoptions.childPath(ANode) == "connection-type")
	{
		updateAccountConnection(AAccount);
	}
	else if (AAccount->isActive() && AAccount->xmppStream()->connection())
	{
		OptionsNode coptions = aoptions.node("connection",aoptions.value("connection-type").toString());
		if (coptions.isChildNode(ANode))
		{
			IConnectionPlugin *plugin = pluginById(coptions.nspace());
			if (plugin)
				plugin->loadConnectionSettings(AAccount->xmppStream()->connection(), coptions);
		}
	}
}

void ConnectionManager::onOptionsOpened()
{
	QNetworkProxy::setApplicationProxy(proxyById(defaultProxy()).proxy);

	// -=Internet Explorer=-
	removeProxy(IEXPLORER_PROXY_REF_UUID);
#ifdef Q_WS_WIN
	IConnectionProxy ieProxy;
	ieProxy.name = tr("<Internet Explorer>");
	QSettings ieProxyReg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",QSettings::NativeFormat);
	if (ieProxyReg.value("ProxyEnable").toBool())
	{
		QString proxyServer = ieProxyReg.value("ProxyServer").toString();
		ieProxy.proxy.setType(QNetworkProxy::HttpProxy);
		ieProxy.proxy.setHostName(proxyServer.split(':').value(0));
		ieProxy.proxy.setPort(proxyServer.split(':').value(1).toInt());
		setProxy(IEXPLORER_PROXY_REF_UUID,ieProxy);
		LogDetaile(QString("[ConnectionManager] Inserted IExplorer connection proxy, host='%1', port='%2'").arg(ieProxy.proxy.hostName()).arg(ieProxy.proxy.port()));
	}
#endif


	// -=Mozilla Firefox=-
	removeProxy(FIREFOX_PROXY_REF_UUID);
	QString ffDir;
	foreach(QString env, QProcess::systemEnvironment())
#ifdef Q_WS_WIN
		if (env.startsWith("APPDATA="))
			ffDir = env.split("=").value(1) + "/Mozilla/Firefox";
#else
		if (env.startsWith("HOME="))
			ffDir = env.split("=").value(1) + "/.mozilla/firefox";
#endif


	if (QFile::exists(QDir(ffDir).absoluteFilePath("profiles.ini")))
	{
		QSettings ffProfies(QDir(ffDir).absoluteFilePath("profiles.ini"),QSettings::IniFormat);
		if (ffProfies.value("Profile0/IsRelative").toBool())
			ffDir = ffDir +"/"+ ffProfies.value("Profile0/Path").toString();
		else
			ffDir = ffProfies.value("Profile0/Path").toString();

		QFile ffPrefs(QDir(ffDir).absoluteFilePath("prefs.js"));
		if (ffPrefs.open(QFile::ReadOnly))
		{
			QString prefs = QString::fromUtf8(ffPrefs.readAll());
			const QString strValueMask = "\\s*user_pref\\(\\s*\\\"%1\\\"\\s*,\\s*\\\"(.*)\\\"\\s*\\)\\s*;";
			const QString intValueMask = "\\s*user_pref\\(\\s*\\\"%1\\\"\\s*,\\s*(.*)\\s*\\)\\s*;";

			QRegExp regexp;
			regexp.setMinimal(true);
			regexp.setPattern(intValueMask.arg("network\\.proxy\\.type"));
			if (regexp.indexIn(prefs) != -1 && regexp.cap(1).trimmed().toInt() == 1)
			{
				IConnectionProxy ffProxy;
				ffProxy.name = tr("<Mozilla Firefox>");
				ffProxy.proxy.setType(QNetworkProxy::HttpProxy);

				regexp.setPattern(strValueMask.arg("network\\.proxy\\.http"));
				if (regexp.indexIn(prefs) != -1)
					ffProxy.proxy.setHostName(regexp.cap(1).trimmed());

				regexp.setPattern(intValueMask.arg("network\\.proxy\\.http_port"));
				if (regexp.indexIn(prefs) != -1)
					ffProxy.proxy.setPort(regexp.cap(1).trimmed().toInt());

				setProxy(FIREFOX_PROXY_REF_UUID,ffProxy);
				LogDetaile(QString("[ConnectionManager] Inserted FireFox connection proxy, host='%1', port='%2'").arg(ffProxy.proxy.hostName()).arg(ffProxy.proxy.port()));
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_connectionmanager, ConnectionManager)
