#ifndef SIPMANAGER_H
#define SIPMANAGER_H

#include <QPointer>
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
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/inotifications.h>
#include <interfaces/imainwindow.h>
#include "phonedialerdialog.h"

struct CallNotifyParams
{
	int rosterNotifyId;
	QDateTime contentTime;
	QMap<IViewWidget *, QUuid> contentId;
};

class SipManager :
		public QObject,
		public IPlugin,
		public ISipManager,
		public ISipCallHandler,
		public IStanzaHandler,
		public IStanzaRequestOwner
{
	Q_OBJECT
	Q_INTERFACES(IPlugin ISipManager ISipCallHandler IStanzaHandler IStanzaRequestOwner)
public:
	SipManager();
	virtual ~SipManager();
	// IPlugin
	virtual QObject *instance();
	virtual QUuid pluginUuid() const;
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// ISipManager
	virtual bool isCallsAvailable() const;
	virtual bool isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	// calls
	virtual ISipCall *newCall(const Jid &AStreamJid, const Jid &APhoneJid);
	virtual ISipCall *newCall(const Jid &AStreamJid, const QList<Jid> &ADestinations);
	virtual QList<ISipCall*> findCalls(const Jid &AStreamJid=Jid::null, const Jid &AContactJid=Jid::null, const QString &ASessionId=QString::null) const;
	// call dialogs
	virtual QWidget *showCallWindow(ISipCall *ACall);
	virtual QWidget *showPhoneDialerDialog(const Jid &AStreamJid);
	// SIP registration
	virtual int sipAccountId(const Jid &AStreamJid) const;
	virtual bool setSipAccountRegistration(const Jid &AStreamJid, bool ARegistered);
	// balance
	virtual bool requestBalance(const Jid &AStreamJid);
	virtual QString requestCallCost(const Jid &AStreamJid, const QString &ACurrencyCode, const QString &ANumber, const QDateTime &AStart, qint64 ADuration);
	// devices
	virtual bool updateAvailDevices();
	virtual bool isDevicePresent(ISipDevice::Type AType) const;
	virtual ISipDevice preferedDevice(ISipDevice::Type AType) const;
	virtual QList<ISipDevice> availDevices(ISipDevice::Type AType) const;
	virtual ISipDevice findDevice(ISipDevice::Type AType, int ADeviceId) const;
	virtual ISipDevice findDevice(ISipDevice::Type AType, const QString &AName) const;
	virtual void showSystemSoundPreferences() const;
	// handlers
	virtual void insertSipCallHandler(int AOrder, ISipCallHandler * AHandler);
	virtual void removeSipCallHandler(int AOrder, ISipCallHandler * AHandler);
signals:
	void availDevicesChanged();
	void sipCallCreated(ISipCall *ACall);
	void sipCallDestroyed(ISipCall *ACall);
	void sipAccountRegistrationChanged(int AAccountId, bool ARegistered);
	void sipCallHandlerInserted(int AOrder, ISipCallHandler * AHandler);
	void sipCallHandlerRemoved(int AOrder, ISipCallHandler * AHandler);
	void sipBalanceRecieved(const Jid &AStreamJid, const ISipBalance &ABalance);
	void sipCallCostRecieved(const QString &AId, const ISipCallCost &ACost);
public:
	// ISipCallHandler
	virtual bool handleSipCall(int AOrder, ISipCall *ACall);
	// IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	// IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
public:
	// SipManager internals
	static SipManager *callbackInstance();
	static QString resolveErrorCode(int code);
public:
	// pjsip callbacks
	void onSipRegistrationState(int AAccountId);
	void onIncomingCall(int acc_id, int call_id, void * /* pjsip_rx_data * */rdata);
protected:
	bool createSipStack();
	void destroySipStack();
	bool handleIncomingCall(const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId);
	void registerCallNotify(ISipCall *ACall);
	void showMissedCallNotify(ISipCall *ACall);
	void showNotifyInRoster(ISipCall *ACall,const QString &AIconId, const QString &AFooter);
	void showNotifyInChatWindow(ISipCall *ACall, const QString &AIconId, const QString &ANotify, bool AOpen = false);
	QList<int> findRelatedNotifies(const Jid &AStreamJid, const Jid &AContactJid) const;
	void updateCallButtonStatusIcon(IMetaTabWindow *AWindow) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onCallStateChanged(int AState);
	void onCallDestroyed();
protected slots:
	void onStartVideoCall();
	void onStartPhoneCall();
	void onShutDownStarted();
	void onShowAddContactDialog();
	void onShowPhoneDialerDialog();
	void onCallMenuAboutToShow();
	void onCallMenuAboutToHide();
	void onChatWindowActivated();
	void onVideoCallChatWindowRequested();
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onXmppStreamRemoved(IXmppStream *AXmppStream);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onMetaTabWindowCreated(IMetaTabWindow *AWindow);
	void onMetaTabWindowDestroyed(IMetaTabWindow *AWindow);
	void onMetaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void onViewWidgetContentChanged(const QUuid &AContentId, const QString &AMessage, const IMessageContentOptions &AOptions);
private:
	IPluginManager *FPluginManager;
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	IMetaContacts *FMetaContacts;
	IXmppStreams *FXmppStreams;
	IRosterChanger *FRosterChanger;
	IGateways *FGateways;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageStyles *FMessageStyles;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	INotifications *FNotifications;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	int FSHISipQuery;
	QMap<QString, Jid> FCostRequests;
	QMap<QString, Jid> FBalanceRequests;
private:
	bool FSipStackCreated;
	QMap<Jid, int> FSipAccounts;
	QMultiMap<int, ISipDevice> FAvailDevices;
	QMap<IMetaTabWindow *, Menu *> FCallMenus;
	QMultiMap<int, ISipCallHandler*> FCallHandlers;
	QMap<ISipCall *, CallNotifyParams> FCallNotifyParams;
	QMap<int, IChatWindow *> FMissedCallNotifies;
	QPointer<PhoneDialerDialog> FPhoneDialerDialog;
private:
	static SipManager *inst;
};

#endif // SIPMANAGER_H
