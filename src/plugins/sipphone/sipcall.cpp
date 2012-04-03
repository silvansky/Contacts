#include "sipcall.h"
#include <QVariant>

SipCall::SipCall(CallerRole role) :
	QObject(NULL)
{
	myRole = role;
}

QObject *SipCall::instance()
{
	return this;
}

Jid SipCall::streamJid() const
{
	return callStreamJid;
}

Jid SipCall::contactJid() const
{
	return callContactJid;
}

QList<Jid> SipCall::callDestinations() const
{
	return destinations;
}

void SipCall::call(const Jid &AStreamJid, const QList<Jid> &AContacts) const
{
	// TODO: implementation
}

void SipCall::acceptCall()
{
	// TODO: implementation
}

void SipCall::rejectCall(ISipCall::RejectionCode ACode)
{
	// TODO: implementation
}

ISipCall::CallState SipCall::state() const
{
	return currentState;
}

ISipCall::ErrorCode SipCall::errorCode() const
{
	return currentError;
}

QString SipCall::errorString() const
{
	// TODO: implementation
	return QString::null;
}

ISipCall::CallerRole SipCall::role() const
{
	return myRole;
}

quint32 SipCall::callTime() const
{
	return currentCallTime;
}

QString SipCall::callTimeString() const
{
	// TODO: implementation
	return QString::null;
}

bool SipCall::sendDTMFSignal(QChar ASignal)
{
	// TODO: implementation
	return true;
}

ISipDevice SipCall::activeDevice(ISipDevice::Type AType) const
{
	switch (AType)
	{
	case ISipDevice::DT_CAMERA:
		return camera;
	case ISipDevice::DT_MICROPHONE:
		return microphone;
	case ISipDevice::DT_VIDEO_IN:
		return videoInput;
	case ISipDevice::DT_AUDIO_OUT:
		return audioOutput;
	default:
		return ISipDevice();
	}
}

bool SipCall::setActiveDevice(ISipDevice::Type AType, int ADeviceId)
{
	// TODO: implementation
	return true;
}

ISipCall::DeviceState SipCall::deviceState(ISipDevice::Type AType) const
{
	// TODO: implementation
	return DS_ENABLED;
}

bool SipCall::setDeviceState(ISipDevice::Type AType, ISipCall::DeviceState AState) const
{
	// TODO: implementation
	return true;
}

QVariant SipCall::deviceProperty(ISipDevice::Type AType, int AProperty)
{
	// TODO: implementation
	return QVariant();
}

bool SipCall::setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	// TODO: implementation
	return true;
}

void SipCall::setStreamJid(const Jid &AStreamJid)
{
	callStreamJid = AStreamJid;
}

void SipCall::setContactJid(const Jid &AContactJid)
{
	callContactJid = AContactJid;
}
