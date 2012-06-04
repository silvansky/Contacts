#ifndef SIPCALL_H
#define SIPCALL_H

#include <QMap>
#include <QTimer>
#include <interfaces/isipphone.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>

class SipCall :
		public QObject,
		public ISipCall,
		public IStanzaHandler,
		public IStanzaRequestOwner
{
	Q_OBJECT
	Q_INTERFACES(ISipCall IStanzaHandler IStanzaRequestOwner)
public:
	SipCall(ISipManager *ASipManager, IXmppStream *AXmppStream, const QString &APhoneNumber, const QString &ASessionId);
	SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const Jid &AContactJid, const QString &ASessionId);
	SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const QList<Jid> &ADestinations, const QString &ASessionId);
	virtual ~SipCall();
	virtual QObject *instance();
	// ISipCall
	virtual bool isDirectCall() const;
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual QString sessionId() const;
	virtual QList<Jid> callDestinations() const;
	virtual void startCall();
	virtual void acceptCall();
	virtual void rejectCall(RejectionCode ACode = RC_BYUSER);
	virtual CallerRole role() const;
	virtual CallState state() const;
	virtual ErrorCode errorCode() const;
	virtual QString errorString() const;
	virtual RejectionCode rejectCode() const;
	virtual quint32 callTime() const; // in milliseconds
	virtual QString callTimeString() const;
	virtual bool sendDTMFSignal(QChar ASignal);
	// devices
	virtual ISipDevice activeDevice(ISipDevice::Type AType) const;
	virtual bool setActiveDevice(ISipDevice::Type AType, int ADeviceId);
	virtual ISipDevice::State deviceState(ISipDevice::Type AType) const;
	virtual bool setDeviceState(ISipDevice::Type AType, ISipDevice::State AState);
	virtual QVariant deviceProperty(ISipDevice::Type AType, int AProperty) const;
	virtual bool setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
signals:
	void callDestroyed();
	void stateChanged(int AState);
	void DTMFSignalReceived(QChar ASignal);
	void activeDeviceChanged(int AType);
	void deviceStateChanged(int AType, int AState);
	void devicePropertyChanged(int AType, int AProperty, const QVariant &AValue);
public:
	// IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	// IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
public:
	// SipCall internal
	int callId() const;
	int accountId() const;
	bool acceptIncomingCall(int ACallId);
public:
	// pjsip callbacks
	void onCallState(int call_id, /*pjsip_event **/ void *e);
	void onCallMediaState(int call_id);
	void onCallTsxState(int call_id, /*pjsip_transaction **/void *tsx, /*pjsip_event **/ void *e);
	int onMyPutFrameCallback(/*pjmedia_frame **/void *frame, int w, int h, int stride);
	int onMyPreviewFrameCallback(/*pjmedia_frame **/void *frame, const char* colormodelName, int w, int h, int stride);
public:
	static SipCall *findCallById(int ACallId);
	static QList<ISipCall*> findCalls(const Jid &AStreamJid=Jid::null, const Jid &AContactJid=Jid::null, const QString &ASessionId=QString::null);
protected:
	void init(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const QString &ASessionId);
	void initDevices();
	void setCallState(CallState AState);
	void setCallError(ErrorCode ACode);
	void continueAfterRegistration(bool ARegistered);
	void notifyActiveDestinations(const QString &AType);
	void sipCallTo(const Jid &AContactJid);
protected slots: 
	void onRingTimerTimeout();
	void onRegisteredAtServer(const QString &AAccount);
	void onUnRegisteredAtServer(const QString &AAccount);
	void onRegistraitionAtServerFailed(const QString &AAccount);
private:
	IXmppStream *FXmppStream;
	ISipManager *FSipManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	// i/o devices
	// local camera
	ISipDevice localCamera;
	ISipDevice::State localCameraState;
	QMap<int, QVariant> localCameraProperties;
	// local microphone
	ISipDevice localMicrophone;
	ISipDevice::State localMicrophoneState;
	QMap<int, QVariant> localMicrophoneProperties;
	// remote camera
	ISipDevice remoteCamera;
	ISipDevice::State remoteCameraState;
	QMap<int, QVariant> remoteCameraProperties;
	// remote microphone
	ISipDevice remoteMicrophone;
	ISipDevice::State remoteMicrophoneState;
	QMap<int, QVariant> remoteMicrophoneProperties;
private:
	// pjsip
	int FCallId;
	int FAccountId;
private:
	bool FDirectCall;
	Jid FContactJid;
	QString FSessionId;
	CallerRole FRole;
	CallState FState;
	ErrorCode FErrorCode;
	RejectionCode FRejectCode;
	QString FErrorString;
	QList<Jid> FDestinations;
	QDateTime FStartCallTime;
	QDateTime FStopCallTime;
private:
	int FSHICallAccept;
	QTimer FRingTimer;
	Stanza FAcceptStanza;
	QList<Jid> FActiveDestinations;
	QMap<QString,Jid> FCallRequests;
private:
	static QList<SipCall *> FCallInstances;
};

#endif // SIPCALL_H
