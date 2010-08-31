#ifndef RAMBLERHISTORY_H
#define RAMBLERHISTORY_H

#include <definations/namespaces.h>
#include <definations/optionnodes.h>
#include <definations/optionvalues.h>
#include <definations/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iramblerhistory.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/errorhandler.h>

class RamblerHistory : 
	public QObject,
	public IPlugin,
	public IOptionsHolder,
	public IStanzaRequestOwner,
	public IRamblerHistory
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder IStanzaRequestOwner IRamblerHistory);
public:
	RamblerHistory();
	~RamblerHistory();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return RAMBLERHISTORY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IRamblerHistory
	virtual bool isSupported(const Jid &AStreamJid) const;
	virtual QString loadServerMessages(const Jid &AStreamJid, const IRamblerHistoryRetrieve &ARetrieve);
signals:
	void serverMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages);
	void requestFailed(const QString &AId, const QString &AError);
private:
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	QList<QString> FRetrieveRequests;
};

#endif // RAMBLERHISTORY_H