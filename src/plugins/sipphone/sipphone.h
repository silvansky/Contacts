#ifndef SIPPHONE_H
#define SIPPHONE_H

#include <definitions/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isipphone.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <utils/errorhandler.h>

class SipPhone : 
	public QObject,
	public IPlugin,
	public ISipPhone,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISipPhone IStanzaHandler IStanzaRequestOwner);
public:
	SipPhone();
	~SipPhone();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return SIPPHONE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//ISipPhone
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual ISipStream streamById(const QString &AStreamId) const;
	virtual QString openStream(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool acceptStream(const QString &AStreamId);
	virtual void closeStream(const QString &AStreamId);
signals:
	void streamCreated(const QString &AStreamId);
	void streamStateChanged(const QString &AStreamId, int AState);
	void streamRemoved(const QString &AStreamId);
protected:
	virtual void removeStream(const QString &AStreamId);
private:
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHISipRequest;
	QMap<QString, QString> FOpenRequests;
	QMap<QString, QString> FCloseRequests;
	QMap<QString, QString> FPendingRequests;
private:
	QMap<QString, ISipStream> FStreams;
};

#endif // SIPPHONE_H
