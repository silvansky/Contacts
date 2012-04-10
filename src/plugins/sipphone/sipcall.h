#ifndef SIPCALL_H
#define SIPCALL_H

#include <QObject>
#include <interfaces/isipphone.h>
#include <interfaces/istanzaprocessor.h>

class SipCall :
		public QObject,
		public ISipCall
{
	Q_OBJECT
	Q_INTERFACES(ISipCall)
public:
	SipCall(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor,  const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId);
	SipCall(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QList<Jid> &AContacts, const QString &ASessionId);
public:
	// ISipCall
	virtual QObject *instance();
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
	virtual quint32 callTime() const;
	virtual QString callTimeString() const;
	virtual bool sendDTMFSignal(QChar ASignal);
	// devices
	virtual ISipDevice activeDevice(ISipDevice::Type AType) const;
	virtual bool setActiveDevice(ISipDevice::Type AType, int ADeviceId);
	virtual DeviceState deviceState(ISipDevice::Type AType) const;
	virtual bool setDeviceState(ISipDevice::Type AType, DeviceState AState) const;
	virtual QVariant deviceProperty(ISipDevice::Type AType, int AProperty);
	virtual bool setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
signals:
	void stateChanged(int AState);
	void DTMFSignalReceived(QChar ASignal);
	void activeDeviceChanged(int ADeviceType);
	void deviceStateChanged(ISipDevice::Type AType, DeviceState AState);
	void devicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
public:
	// SipCall internal
public:
	static SipCall * activeCallForId(int id);
public:
	// pjsip callbacks
	void onCallState(int call_id, /*pjsip_event **/ void *e);
	void onCallMediaState(int call_id);
	void onCallTsxState(int call_id, /*pjsip_transaction **/void * tsx, /*pjsip_event **/ void *e);
	int onMyPutFrameCallback(int call_id, /*pjmedia_frame **/void *frame, int w, int h, int stride);
	int onMyPreviewFrameCallback(/*pjmedia_frame **/void *frame, const char* colormodelName, int w, int h, int stride);
protected:
	void sipCallTo(const Jid &AContactJid);
private:
	ISipManager *FSipManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	// i/o devices
	ISipDevice camera;
	ISipDevice microphone;
	ISipDevice videoInput;
	ISipDevice audioOutput;
private:
	// pjsip
	int currentCall;
	int accountId;
private:
	Jid FStreamJid;
	Jid FContactJid;
	QString FSessionId;
	CallerRole myRole;
	QList<Jid> FDestinations;
	CallState currentState;
	ErrorCode currentError;
	quint32 currentCallTime;
private:
	static QMap<int, SipCall*> activeCalls;
};

#endif // SIPCALL_H
