#ifndef TCPTUNNEL_H
#define TCPTUNNEL_H

#include <QMap>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/igateways.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <utils/log.h>
#include <utils/errorhandler.h>
#include "tunnelthread.h"

#define TCPTUNNEL_UUID "{2B8BB093-AF67-4c13-ADFA-2EB56B9DDE39}"

class TcpTunnel : 
	public QObject,
	public IPlugin,
	public IStanzaHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaHandler);
public:
	TcpTunnel();
	~TcpTunnel();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return TCPTUNNEL_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
protected:
	void onShutdownStarted();
	void onTunnelThreadConnected(const QString &AKey);
	void onTunnelThreadDisconnected(const QString &ACondition);
private:
	IGateways *FGateways;
	IXmppStreams *FXmppStreams;
	IPluginManager *FPluginManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHIConnect;
	QList<TunnelThread *> FTunnels;
	QMap<TunnelThread *, Stanza> FRequests;
};

#endif // TCPTUNNEL_H
