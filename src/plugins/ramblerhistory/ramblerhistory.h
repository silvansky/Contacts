#ifndef RAMBLERHISTORY_H
#define RAMBLERHISTORY_H

#include <definitions/resources.h>
#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iramblerhistory.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iroster.h>
#include <utils/log.h>
#include <utils/stanza.h>
#include <utils/datetime.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include "viewhistorywindow.h"
#include "historyoptionswidget.h"

class RamblerHistory : 
	public QObject,
	public IPlugin,
	public IOptionsHolder,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IRamblerHistory
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder IStanzaHandler IStanzaRequestOwner IRamblerHistory);
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
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IRamblerHistory
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual bool isSupported(const Jid &AStreamJid) const;
	virtual IHistoryStreamPrefs historyPrefs(const Jid &AStreamJid) const;
	virtual IHistoryItemPrefs historyItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const;
	virtual QString setHistoryPrefs(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs);
	virtual QString loadServerMessages(const Jid &AStreamJid, const IHistoryRetrieve &ARetrieve);
	virtual QWidget *showViewHistoryWindow(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void requestCompleted(const QString &AId);
	void requestFailed(const QString &AId, const QString &AError);
	//History preferences
	void historyPrefsChanged(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs);
	void historyItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IHistoryItemPrefs &APrefs);
	//Server History
	void serverMessagesLoaded(const QString &AId, const IHistoryMessages &AMessages);
protected:
	QString loadServerPrefs(const Jid &AStreamJid);
	void applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem);
	ViewHistoryWindow *findViewWindow(IRoster *ARoster, const Jid &AContactJid) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onRosterRemoved(IRoster *ARoster);
	void onViewHistoryWindowDestroyed();
private:
	IXmppStreams *FXmppStreams;
	IRosterPlugin *FRosterPlugin;
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid,int> FSHIPrefsUpdate;
private:
	QMap<QString,Jid> FRetrieveRequests;
	QMap<QString,Jid> FPrefsLoadRequests;
	QMap<QString,Jid> FPrefsSaveRequests;
private:
	QMap<Jid,IHistoryStreamPrefs> FHistoryPrefs;
	QMultiMap<IRoster *, ViewHistoryWindow *> FViewWindows;
};

#endif // RAMBLERHISTORY_H
