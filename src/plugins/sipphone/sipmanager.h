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
	SipManager();
	~SipManager();
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
	virtual ISipCall *newCall(const Jid &AStreamJid, const QList<Jid> &AContacts);
	virtual QList<ISipCall*> findCalls(const Jid & AStreamJid=Jid::null, const Jid AContactJid=Jid::null, const QString &ASessionId=QString::null) const;
	// SIP registration
	virtual bool isRegisteredAtServer(const Jid &AStreamJid) const;
	virtual bool registerAtServer(const Jid &AStreamJid, const QString & APassword);
	virtual bool unregisterAtServer(const Jid &AStreamJid, const QString & APassword);
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
signals:
	void sipCallCreated(ISipCall * ACall);
	void sipCallDestroyed(ISipCall * ACall);
	void registeredAtServer(const Jid &AStreamJid);
	void unregisteredAtServer(const Jid &AStreamJid);
	void failedToRegisterAtServer(const Jid &AStreamJid);
	void availDevicesChanged(int ADeviceType);
	void sipCallHandlerInserted(int AOrder, ISipCallHandler * AHandler);
	void sipCallHandlerRemoved(int AOrder, ISipCallHandler * AHandler);
public:
	// SipManager internals
	static SipManager * callbackInstance();
	// pjsip callbacks
	void onRegState(int acc_id);
	void onRegState2(int acc_id, void * /* pjsua_reg_info * */info);
	void onIncomingCall(int acc_id, int call_id, void * /* pjsip_rx_data * */rdata);
protected:
	bool handleIncomingCall(const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId);
	bool initStack(const QString &ASipServer, int ASipPort, const Jid &ASipUser, const QString &ASipPassword);
protected slots:
	void onCallDestroyed(QObject*);
private:
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	IRostersView *FRostersView;
	IMetaContacts *FMetaContacts;
private:
	int FSHISipQuery;
private:
	QMap<int, ISipCallHandler*> handlers;
	QList<ISipCall*> calls;
	QMap<Jid, int> accountIds;
	static SipManager * inst;
	bool accRegistered;
};

#endif // SIPMANAGER_H
