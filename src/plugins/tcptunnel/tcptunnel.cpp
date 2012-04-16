#include "tcptunnel.h"

#define SHC_CONNECT  "/iq[@type='set']/connect[@xmlns='" NS_RAMBLER_TCPTUNNEL_CONNECT "']"

TcpTunnel::TcpTunnel()
{
	FGateways = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
}

TcpTunnel::~TcpTunnel()
{

}

void TcpTunnel::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("TCP Tunnel");
	APluginInfo->description = tr("Allow to make TCP tunnels for transport connections");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(GATEWAYS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool TcpTunnel::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

	return FStanzaProcessor!=NULL && FGateways!=NULL;
}

bool TcpTunnel::initObjects()
{
	ErrorHandler::addErrorItem("proxy-xml-not-well-formed", ErrorHandler::CANCEL,
		ErrorHandler::BAD_REQUEST, tr("Received not well formed XML from tunnel proxy"),NS_RAMBLER_TCPTUNNEL_CONNECT);

	ErrorHandler::addErrorItem("proxy-invalid-session-key", ErrorHandler::CANCEL,
		ErrorHandler::BAD_REQUEST, tr("Failed to receive session key from tunnel proxy"),NS_RAMBLER_TCPTUNNEL_CONNECT);

	ErrorHandler::addErrorItem("proxy-connect-error", ErrorHandler::CANCEL,
		ErrorHandler::REMOUTE_SERVER_ERROR, tr("Failed to connect to tunnel proxy host"),NS_RAMBLER_TCPTUNNEL_CONNECT);

	ErrorHandler::addErrorItem("remote-connect-error", ErrorHandler::CANCEL,
		ErrorHandler::REMOUTE_SERVER_ERROR, tr("Failed to connect to remote host"),NS_RAMBLER_TCPTUNNEL_CONNECT);

	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_DEFAULT;
		handle.conditions.append(SHC_CONNECT);
		handle.direction = IStanzaHandle::DirectionIn;
		FSHIConnect = FStanzaProcessor->insertStanzaHandle(handle);
	}
	return true;
}

bool TcpTunnel::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId == FSHIConnect)
	{
		if (FGateways && FGateways->streamServices(AStreamJid).contains(AStanza.from()))
		{
			ConnectRequest request;
			QDomElement connectElem = AStanza.firstElement("connect",NS_RAMBLER_TCPTUNNEL_CONNECT);

			QDomElement remoteElem = connectElem.firstChildElement("remote");
			request.remoteHost = remoteElem.firstChildElement("host").text();
			request.remotePort = remoteElem.firstChildElement("port").text().toInt();
			request.remoteEncrypted = QVariant(remoteElem.firstChildElement("encrypted").text()).toBool();

			QDomElement proxyElem = connectElem.firstChildElement("proxy");
			request.proxyHost = proxyElem.firstChildElement("host").text();
			request.proxyPort = proxyElem.firstChildElement("port").text().toInt();
			request.proxyEncrypted = QVariant(proxyElem.firstChildElement("encrypted").text()).toBool();

			IXmppStream *xmppStream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
			if (xmppStream && xmppStream->connection())
			{
				IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(xmppStream->connection()->instance());
				if (defConnection)
					request.connectProxy = defConnection->proxy();
			}

			if (request.isValid())
			{
				AAccept = true;
				LogDetail(QString("[TcpTunnel] Connect request received from %1, id=%2, remote=%3:%4 (enc=%7), proxy=%5:%6 (enc=%8)").arg(AStanza.from(),AStanza.id(),request.remoteHost).arg(request.remotePort).arg(request.proxyHost).arg(request.proxyPort).arg(request.proxyEncrypted).arg(request.remoteEncrypted));
				TunnelThread *tunnel = new TunnelThread(request,this);
				connect(tunnel,SIGNAL(connected(const QString &)),SLOT(onTunnelThreadConnected(const QString &)),Qt::QueuedConnection);
				connect(tunnel,SIGNAL(disconnected(const QString &)),SLOT(onTunnelThreadDisconnected(const QString &)),Qt::QueuedConnection);
				FTunnels.append(tunnel);
				FRequests.insert(tunnel,AStanza);
				tunnel->start();
			}
			else
			{
				LogError(QString("[TcpTunnel] Invalid connect request received from %1, id=%2, remote=%3:%4, proxy=%5:%6").arg(AStanza.from(),AStanza.id(),request.remoteHost).arg(request.remotePort).arg(request.proxyHost).arg(request.proxyPort));
			}
			return true;
		}
	}
	return false;
}

void TcpTunnel::onShutdownStarted()
{
	foreach(TunnelThread *tunnel, FTunnels)
	{
		tunnel->abort();
		FPluginManager->delayShutdown();
	}
}

void TcpTunnel::onTunnelThreadConnected(const QString &AKey)
{
	TunnelThread *tunnel = qobject_cast<TunnelThread *>(sender());
	if (tunnel && FRequests.contains(tunnel))
	{
		Stanza request = FRequests.take(tunnel);
		Stanza result = FStanzaProcessor->makeReplyResult(request);
		QDomElement connectElem = result.addElement("connect",NS_RAMBLER_TCPTUNNEL_CONNECT);
		connectElem.appendChild(result.createElement("session-key")).appendChild(result.createTextNode(AKey));
		if (FStanzaProcessor->sendStanzaOut(request.to(),result))
		{
			LogDetail(QString("[TcpTunnel] TCP tunnel established, key=%1, id=%2").arg(AKey,request.id()));
		}
		else
		{
			tunnel->abort();
			LogError(QString("[TcpTunnel] Failed to send connected result, key=%1, id=%2").arg(AKey,request.id()));
		}
	}
}

void TcpTunnel::onTunnelThreadDisconnected(const QString &ACondition)
{
	TunnelThread *tunnel = qobject_cast<TunnelThread *>(sender());
	if (tunnel)
	{
		if (FRequests.contains(tunnel))
		{
			Stanza request = FRequests.take(tunnel);
			ErrorHandler err(ACondition,NS_RAMBLER_TCPTUNNEL_CONNECT);
			Stanza error = FStanzaProcessor->makeReplyError(request,err);
			error.firstElement("error").appendChild(error.createElement("recipient-unavailable",EHN_DEFAULT));
			FStanzaProcessor->sendStanzaOut(request.to(),error);
			LogError(QString("[TcpTunnel] Failed to establish TCP tunnel, error=%1, id=%2").arg(ACondition,request.id()));
		}
		else
		{
			LogDetail(QString("[TcpTunnel] TCP tunnel closed, key=%1").arg(tunnel->sessionKey()));
		}
		FTunnels.removeAll(tunnel);
		FPluginManager->continueShutdown();
		tunnel->deleteLater();
	}
}

Q_EXPORT_PLUGIN2(plg_tcptunnel, TcpTunnel)
