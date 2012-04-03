#include "sipcall.h"
#include <QVariant>

SipCall::SipCall(QObject *parent) :
	QObject(parent)
{
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
	// TODO: implementation
	return CS_NONE;
}

ISipCall::ErrorCode SipCall::errorCode() const
{
	// TODO: implementation
	return EC_NONE;
}

QString SipCall::errorString() const
{
	// TODO: implementation
	return QString::null;
}

ISipCall::CallerRole SipCall::role() const
{
	// TODO: implementation
	return CR_INITIATOR;
}

quint32 SipCall::callTime() const
{
	// TODO: implementation
	return 0;
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
	// TODO: implementation
	return ISipDevice();
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
