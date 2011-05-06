#ifndef RAMBLERHISTORY_H
#define RAMBLERHISTORY_H

#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iramblerhistory.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iroster.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include <utils/custombordercontainer.h>
#include "viewhistorywindow.h"

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
	virtual QWidget *showViewHistoryWindow(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void serverMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages);
	void requestFailed(const QString &AId, const QString &AError);
protected:
	ViewHistoryWindow *findViewWindow(IRoster *ARoster, const Jid &AContactJid) const;
protected slots:
	void onRosterRemoved(IRoster *ARoster);
	void onViewHistoryWindowDestriyed();
private:
	IRosterPlugin *FRosterPlugin;
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	QList<QString> FRetrieveRequests;
	QMultiMap<IRoster *, ViewHistoryWindow *> FViewWindows;
};

#endif // RAMBLERHISTORY_H
