#ifndef ISIPPHONE_H
#define ISIPPHONE_H

#include <QUuid>
#include <QDateTime>
#include <utils/jid.h>
#include <utils/errorhandler.h>

#define SIPMANAGER_UUID "{582D16F6-FDB1-4ADF-9555-17B99234179E}"

struct ISipCallCost
{
	float cost;
	QString phone;
	QDateTime start;
	qint64 duration;
	QString currency;
	QString city;
	QString cityCode;
	QString country;
	QString countryCode;
};

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
		LCP_AUTO_START,           /* bool */
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
		index = -1;
		name = QString::null;
	}

	Type type;
	int index;
	QString name;

	bool isNull() const {
		return type==DT_UNDEFINED;
	}
	bool operator==(const ISipDevice &AOther) const {
		return AOther.type==type && AOther.name==name;
	}
	bool operator!=(const ISipDevice &AOther) const {
		return !operator==(AOther);
	}
};

class ISipCall
{
public:
	enum CallerRole
	{
		CR_INITIATOR,
		CR_RESPONDER
	};

	enum CallState
	{
		CS_INIT,
		CS_CALLING,
		CS_CONNECTING,
		CS_TALKING,
		CS_FINISHED,
		CS_ERROR
	};

	enum RejectionCode
	{
		RC_EMPTY,
		RC_BUSY,
		RC_BYUSER,
		RC_NOHANDLER
	};

	enum ErrorCode
	{
		EC_EMPTY,
		EC_BUSY,
		EC_NOTAVAIL,
		EC_NOANSWER,
		EC_REJECTED,
		EC_CONNECTIONERR
	};

	virtual QObject *instance() = 0;
	virtual bool isPhoneCall() const =0;
	virtual Jid streamJid() const = 0;
	virtual Jid contactJid() const = 0;
	virtual QString sessionId() const = 0;
	virtual QList<Jid> callDestinations() const = 0;
	virtual void startCall() = 0;
	virtual void acceptCall() = 0;
	virtual void rejectCall(RejectionCode ACode = RC_BYUSER) = 0;
	virtual void destroyCall() =0;
	virtual CallerRole role() const = 0;
	virtual CallState state() const = 0;
	virtual ErrorCode errorCode() const = 0;
	virtual QString errorString() const = 0;
	virtual RejectionCode rejectCode() const =0;
	virtual quint32 callTime() const = 0;
	virtual QString callTimeString() const = 0;
	virtual bool sendDTMFSignal(QChar ASignal) = 0;
	// devices
	virtual ISipDevice::State deviceState(ISipDevice::Type AType) const = 0;
	virtual bool setDeviceState(ISipDevice::Type AType, ISipDevice::State AState) = 0;
	virtual QVariant deviceProperty(ISipDevice::Type AType, int AProperty) const = 0;
	virtual bool setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue) = 0;
protected:
	virtual void callDestroyed() =0;
	virtual void stateChanged(int AState) = 0;
	virtual void DTMFSignalReceived(QChar ASignal) = 0;
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
	virtual bool isCallsAvailable() const =0;
	virtual bool isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const = 0;
	// calls
	virtual ISipCall *newCall(const Jid &AStreamJid, const Jid &APhoneJid) =0;
	virtual ISipCall *newCall(const Jid &AStreamJid, const QList<Jid> &ADestinations) = 0;
	virtual QList<ISipCall*> findCalls(const Jid &AStreamJid=Jid::null, const Jid &AContactJid=Jid::null, const QString &ASessionId=QString::null) const = 0;
	// SIP registration
	virtual int sipAccountId(const Jid &AStreamJid) const =0;
	virtual bool setSipAccountRegistration(const Jid &AStreamJid, bool ARegistered) =0;
	// balance
	virtual bool requestAccountBalance(const Jid &AStreamJid) =0;
	virtual QString requestCallCost(const Jid &AStreamJid, const QString &ACurrency, const QString &APhone, const QDateTime &AStart, qint64 ADuration) =0;
	// devices
	virtual bool updateAvailDevices() =0;
	virtual bool isDevicePresent(ISipDevice::Type AType) const =0;
	virtual ISipDevice preferedDevice(ISipDevice::Type AType) const =0;
	virtual QList<ISipDevice> availDevices(ISipDevice::Type AType) const = 0;
	virtual ISipDevice findDevice(ISipDevice::Type AType, int ADeviceId) const = 0;
	virtual ISipDevice findDevice(ISipDevice::Type AType, const QString &AName) const =0;
	virtual void showSystemSoundPreferences() const = 0;
	// handlers
	virtual void insertSipCallHandler(int AOrder, ISipCallHandler *AHandler) = 0;
	virtual void removeSipCallHandler(int AOrder, ISipCallHandler *AHandler) = 0;
protected:
	virtual void availDevicesChanged() = 0;
	virtual void sipCallCreated(ISipCall *ACall) = 0;
	virtual void sipCallDestroyed(ISipCall *ACall) = 0;
	virtual void sipAccountRegistrationChanged(int AAccountId, bool ARegistered) =0;
	virtual void sipCallHandlerInserted(int AOrder, ISipCallHandler *AHandler) = 0;
	virtual void sipCallHandlerRemoved(int AOrder, ISipCallHandler *AHandler) = 0;
	virtual void accountBalanceRecieved(const Jid &AStreamJid, float ABalance, const QString &ACurrency) =0;
	virtual void callCostRecieved(const QString &AId, const ISipCallCost &ACost, const ErrorHandler &AError) =0;
};

Q_DECLARE_INTERFACE(ISipCall,"Virtus.Plugin.ISipCall/1.0")
Q_DECLARE_INTERFACE(ISipCallHandler,"Virtus.Plugin.ISipCallHandler/1.0")
Q_DECLARE_INTERFACE(ISipManager,"Virtus.Plugin.ISipManager/1.0")

#endif //ISIPPHONE_H
