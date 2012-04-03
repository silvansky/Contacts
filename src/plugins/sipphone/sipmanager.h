#ifndef SIPMANAGER_H
#define SIPMANAGER_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/isipphone.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/irostersview.h>
#include <interfaces/inotifications.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/igateways.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/imessagestyles.h>

class SipManager :
		public QObject,
		public IPlugin,
		public ISipManager,
		public IStanzaHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin ISipManager IStanzaHandler)
public:
	explicit SipManager(QObject *parent);
	// IPlugin
	virtual QObject *instance();
	virtual QUuid pluginUuid() const;
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// ISipManager
	virtual bool isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	// calls
	virtual ISipCall * newCall();
	virtual QList<ISipCall*> findCalls(const Jid & AStreamJid = Jid::null);
	// prices
	// TODO
	// devices
	virtual QList<ISipDevice> availDevices(ISipDevice::Type AType) const;
	virtual ISipDevice getDevice(ISipDevice::Type AType, int ADeviceId) const;
	virtual void showSystemSoundPreferences() const;
	// handlers
	virtual void insertSipCallHandler(int AOrder, ISipCallHandler * AHandler);
	virtual void removeSipCallHandler(int AOrder, ISipCallHandler * AHandler);
	// IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
protected:
	// SipManager internals
	bool handleIncomingCall(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void sipCallCreated(ISipCall * ACall);
	void sipCallDestroyed(ISipCall * ACall);
	void availDevicesChanged(int ADeviceType);
	void sipCallHandlerInserted(int AOrder, ISipCallHandler * AHandler);
	void sipCallHandlerRemoved(int AOrder, ISipCallHandler * AHandler);
	
protected slots:
	void onCallDestroyed(QObject*);
private:
	IGateways *FGateways;
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	INotifications *FNotifications;
	IRosterChanger *FRosterChanger;
	IRostersView *FRostersView;
	IMetaContacts *FMetaContacts;
	IPresencePlugin *FPresencePlugin;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
private:
	QMap<int, QString> FIncomingNotifies;
	QMap<int, IChatWindow *> FMissedNotifies;
private:
	int FSHISipRequest;
	QMap<QString, QString> FOpenRequests;
	QMap<QString, QString> FCloseRequests;
	QMap<QString, QString> FPendingRequests;
private:
	QMap<int, ISipCallHandler*> handlers;
	QList<ISipCall*> calls;
	
};

#endif // SIPMANAGER_H
