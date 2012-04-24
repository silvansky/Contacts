#ifndef SIPCALL_H
#define SIPCALL_H

#include <QMap>
#include <QTimer>
#include <interfaces/isipphone.h>
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
	SipCall(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId);
	SipCall(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QList<Jid> &ADestinations, const QString &ASessionId);
	~SipCall();
	virtual QObject *instance();
	// ISipCall
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
	virtual quint32 callTime() const; // in seconds
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
	void activeDeviceChanged(ISipDevice::Type ADeviceType);
	void deviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState);
	void devicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant &AValue);
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
	static SipCall *findCallById(int ACallId);
	static QList<ISipCall*> findCalls(const Jid &AStreamJid=Jid::null, const Jid &AContactJid=Jid::null, const QString &ASessionId=QString::null);
public:
	// pjsip callbacks
	void onCallState(int call_id, /*pjsip_event **/ void *e);
	void onCallMediaState(int call_id);
	void onCallTsxState(int call_id, /*pjsip_transaction **/void * tsx, /*pjsip_event **/ void *e);
	int onMyPutFrameCallback(int call_id, /*pjmedia_frame **/void *frame, int w, int h, int stride);
	int onMyPreviewFrameCallback(/*pjmedia_frame **/void *frame, const char* colormodelName, int w, int h, int stride);
protected:
	void init(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QString &ASessionId);
	void setCallState(CallState AState);
	void setCallError(ErrorCode ACode, const QString &AMessage);
	void continueAfterRegistration(bool ARegistered);
	void notifyActiveDestinations(const QString &AType);
	void sipCallTo(const Jid &AContactJid);
protected slots: 
	void onRingTimerTimeout();
	void onRegisteredAtServer(const QString &AAccount);
	void onUnRegisteredAtServer(const QString &AAccount);
	void onRegistraitionAtServerFailed(const QString &AAccount);
private:
	ISipManager *FSipManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	// i/o devices
	// camera (my video)
	ISipDevice localCamera;
	ISipDevice::State localCameraState;
	QMap<int, QVariant> localCameraProperties;
	// microphone (my sound)
	ISipDevice localMicrophone;
	ISipDevice::State localMicrophoneState;
	QMap<int, QVariant> localMicrophoneProperties;
	// video input (remote video)
	ISipDevice remoteCamera;
	ISipDevice::State remoteCameraState;
	QMap<int, QVariant> remoteCameraProperties;
	// audio output (remote sound)
	ISipDevice remoteMicrophone;
	ISipDevice::State remoteMicrophoneState;
	QMap<int, QVariant> remoteMicrophoneProperties;
private:
	// pjsip
	int FCallId;
	int FAccountId;
private:
	Jid FStreamJid;
	Jid FContactJid;
	QString FSessionId;
	CallerRole FRole;
	CallState FState;
	ErrorCode FErrorCode;
	QString FErrorString;
	QList<Jid> FDestinations;
	QDateTime FStartCallTime;
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
