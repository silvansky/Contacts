#ifndef ISIPPHONE_H
#define ISIPPHONE_H

#include <QUuid>
#include <QDateTime>
#include <utils/jid.h>

#define SIPPHONE_UUID   "{28686B71-6E29-4065-8D2E-6116F2491394}"
#define SIPMANAGER_UUID "{582D16F6-FDB1-4ADF-9555-17B99234179E}"

struct ISipStream 
{
	enum Kind {
		SK_INITIATOR,
		SK_RESPONDER
	};
	enum State {
		SS_OPEN,
		SS_OPENED,
		SS_CLOSE,
		SS_CLOSED
	};
	enum ErrorFlag
	{
		EF_NO_ERROR = 0,
		EF_REGFAIL,
		EF_DEVERR
	};
	ISipStream()
	{
		kind = SK_INITIATOR;
		state = SS_CLOSED;
		errFlag = EF_NO_ERROR;
		timeout = false;
	}
	int kind;
	int state;
	int errFlag;
	QString failReason;
	QString sid;
	Jid streamJid;
	Jid contactJid;
	bool timeout;
	QUuid contentId;
	QDateTime startTime;
};

class ISipPhone
{
public:
	virtual QObject *instance() =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<QString> streams() const =0;
	virtual ISipStream streamById(const QString &AStreamId) const =0;
	virtual QString findStream(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString openStream(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual bool acceptStream(const QString &AStreamId) =0;
	virtual void closeStream(const QString &AStreamId) =0;
protected:
	virtual void streamCreated(const QString &AStreamId) =0;
	virtual void streamStateChanged(const QString &AStreamId, int AState) =0;
	virtual void streamRemoved(const QString &AStreamId) =0;
};

///////////////////////////////////////////////////////////////////////////////////////////////

struct ISipDevice
{
	enum Type
	{
		DT_UNDEFINED,
		DT_LOCAL_CAMERA,
		DT_REMOTE_CAMERA,
		DT_LOCAL_MICROPHONE,
		DT_REMOTE_MICROPHONE
	};

	enum State
	{
		DS_UNAVAIL,
		DS_ENABLED,
		DS_DISABLED
	};

	enum LocalCameraProperty
	{
		LCP_CURRENTFRAME,         /* QPixmap, readonly */
		LCP_AVAIL_RESOLUTIONS,    /* QList<QSize>, readonly */	/* unused */
		LCP_RESOLUTION,           /* QSize */  /* unused */
		LCP_BRIGHTNESS,           /* float */  /* unused */
		LCP_USER                  /* for user defined properties, add new before this */
	};

	enum RemoteCameraProperty
	{
		RCP_CURRENTFRAME,         /* QPixmap, readonly */
		RCP_USER                  /* for user defined properties, add new before this */
	};

	enum LocalMicrophoneProperty
	{
		LMP_VOLUME,               /* float > 0.0 */
		LMP_MAX_VOLUME,           /* float, readonly */
		LMP_USER                  /* for user defined properties, add new before this */
	};

	enum RemoteMicrophoneProperty
	{
		RMP_VOLUME,               /* float > 0.0 */
		RMP_MAX_VOLUME,           /* float, readonly */
		RMP_USER                  /* for user defined properties, add new before this */
	};

	ISipDevice()
	{
		type = DT_UNDEFINED;
		id = -1;
		name = QString::null;
	}

	Type type;
	int id;
	QString name;
};

class ISipCall
{
public:
	enum CallState
	{
		CS_NONE,
		CS_CALLING,
		CS_CONNECTING,
		CS_TALKING,
		CS_FINISHED,
		CS_ERROR
	};

	enum ErrorCode
	{
		EC_NONE,
		EC_BUSY,
		EC_NOTAVAIL,
		EC_NOANSWER,
		EC_REJECTED,
		EC_CONNECTIONERR
	};

	enum CallerRole
	{
		CR_INITIATOR,
		CR_RESPONDER
	};

	enum RejectionCode
	{
		RC_BYUSER,
		RC_BUSY,
		RC_NOHANDLER
	};

	virtual QObject *instance() = 0;
	virtual bool isDirectCall() const =0;
	virtual Jid streamJid() const = 0;
	virtual Jid contactJid() const = 0;
	virtual QString sessionId() const = 0;
	virtual QList<Jid> callDestinations() const = 0;
	virtual void startCall() = 0;
	virtual void acceptCall() = 0;
	virtual void rejectCall(RejectionCode ACode = RC_BYUSER) = 0;
	virtual CallerRole role() const = 0;
	virtual CallState state() const = 0;
	virtual ErrorCode errorCode() const = 0;
	virtual QString errorString() const = 0;
	virtual quint32 callTime() const = 0; // in milliseconds
	virtual QString callTimeString() const = 0;
	virtual bool sendDTMFSignal(QChar ASignal) = 0;
	// devices
	virtual ISipDevice activeDevice(ISipDevice::Type AType) const = 0;
	virtual bool setActiveDevice(ISipDevice::Type AType, int ADeviceId) = 0;
	virtual ISipDevice::State deviceState(ISipDevice::Type AType) const = 0;
	virtual bool setDeviceState(ISipDevice::Type AType, ISipDevice::State AState) = 0;
	virtual QVariant deviceProperty(ISipDevice::Type AType, int AProperty) const = 0;
	virtual bool setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue) = 0;
protected:
	virtual void callDestroyed() =0;
	virtual void stateChanged(int AState) = 0;
	virtual void DTMFSignalReceived(QChar ASignal) = 0;
	virtual void activeDeviceChanged(int AType) = 0;
	virtual void deviceStateChanged(int AType, int AState) = 0;
	virtual void devicePropertyChanged(int AType, int AProperty, const QVariant &AValue) = 0;
};

class ISipCallHandler
{
public:
	virtual bool handleSipCall(int AOrder, ISipCall *ACall) = 0;
};

class ISipManager
{
public:
	virtual QObject *instance() = 0;
	virtual bool isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const = 0;
	// calls
	virtual ISipCall *newCall(const Jid &AStreamJid, const QList<Jid> &ADestinations) = 0;
	virtual QList<ISipCall*> findCalls(const Jid &AStreamJid=Jid::null, const Jid &AContactJid=Jid::null, const QString &ASessionId=QString::null) const = 0;
	// SIP registration
	virtual int registeredAccountId(const Jid &AStreamJid) const = 0;
	virtual bool isRegisteredAtServer(const Jid &AStreamJid) const = 0;
	virtual bool registerAtServer(const Jid &AStreamJid) = 0;
	virtual bool unregisterAtServer(const Jid &AStreamJid) = 0;
	// devices
	virtual void showSystemSoundPreferences() const = 0;
	virtual QList<ISipDevice> availDevices(ISipDevice::Type AType, bool ARefresh = false) const = 0;
	virtual ISipDevice getDevice(ISipDevice::Type AType, int ADeviceId) const = 0;
	// handlers
	virtual void insertSipCallHandler(int AOrder, ISipCallHandler *AHandler) = 0;
	virtual void removeSipCallHandler(int AOrder, ISipCallHandler *AHandler) = 0;
protected:
	virtual void availDevicesChanged(int AType) = 0;
	virtual void sipCallCreated(ISipCall *ACall) = 0;
	virtual void sipCallDestroyed(ISipCall *ACall) = 0;
	virtual void registeredAtServer(const QString &AAccount) = 0;
	virtual void unregisteredAtServer(const QString &AAccount) = 0;
	virtual void registrationAtServerFailed(const QString &AAccount) = 0;
	virtual void sipCallHandlerInserted(int AOrder, ISipCallHandler *AHandler) = 0;
	virtual void sipCallHandlerRemoved(int AOrder, ISipCallHandler *AHandler) = 0;
};

Q_DECLARE_INTERFACE(ISipCall,"Virtus.Plugin.ISipCall/1.0")
Q_DECLARE_INTERFACE(ISipCallHandler,"Virtus.Plugin.ISipCallHandler/1.0")
Q_DECLARE_INTERFACE(ISipManager,"Virtus.Plugin.ISipManager/1.0")
Q_DECLARE_INTERFACE(ISipPhone,"Virtus.Plugin.ISipPhone/1.0")

#endif //ISIPPHONE_H
