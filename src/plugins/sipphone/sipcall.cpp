#include "sipcall.h"
#include <QVariant>
#include <pjsua.h>
#include <assert.h>
#include <utils/log.h>

// 0 - disable, 1 - enable
#define HAS_VIDEO_SUPPORT	1

#define SIP_DOMAIN		"vsip.rambler.ru"

#define DEFAULT_CAP_DEV		PJMEDIA_VID_DEFAULT_CAPTURE_DEV
#define DEFAULT_REND_DEV	PJMEDIA_VID_DEFAULT_RENDER_DEV

QMap<int, SipCall*> SipCall::activeCalls;

SipCall::SipCall(CallerRole role, ISipManager *manager) :
	QObject(NULL)
{
	sipManager = manager;
	myRole = role;
	currentState = (myRole == CR_INITIATOR) ? CS_NONE : CS_CALLING;
	currentError = EC_NONE;
	currentCall = -1;
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
	// step 1: check registration at SIP server
	// step 2: ask SIP server to connect
	// step 3:

	if (sipManager->isRegisteredAtServer(AStreamJid))
	{
		pj_status_t status;
		char uriTmp[512];
		const char* uriToCall = AContacts.first().full().toUtf8().constData();

		pj_ansi_sprintf(uriTmp, "sip:%s", uriToCall);
		pj_str_t uri = pj_str((char*)uriTmp);

		pj_assert(currentCall == -1);

		pjsua_call_setting call_setting;
		pjsua_call_setting_default(&call_setting);

#if defined(HAS_VIDEO_SUPPORT)
		call_setting.vid_cnt = HAS_VIDEO_SUPPORT;
#else
		call_setting.vid_cnt = 0;
#endif
		//call_setting.vid_cnt = 0;//(vidEnabled_->checkState()==Qt::Checked);
		call_setting.aud_cnt = 1;

		// NULL SOUND
		//int capture_dev = -99;
		//int playback_dev = -99;
		//pjsua_get_snd_dev(&capture_dev, &playback_dev);
		//status = pjsua_set_null_snd_dev();
		//pjsua_set_snd_dev(-99, playback_dev);

		pjsua_call_id id = currentCall;
		status = pjsua_call_make_call(accountId, &uri, &call_setting, NULL, NULL, &id);
		//status = PJMEDIA_EAUD_NODEFDEV;
		if (status != PJ_SUCCESS)
		{
			if(status == PJMEDIA_EAUD_NODEFDEV)
			{
				//emit signal_DeviceError();
//				emit signal_InviteStatus(false, 2, tr("Failed to find default audio device"));
			}
//			showError("make call", status);
			return;
		}
//		emit signal_InviteStatus(true, 0, "");
	}
}

void SipCall::acceptCall()
{
	// TODO: implementation
}

void SipCall::rejectCall(ISipCall::RejectionCode ACode)
{
	Q_UNUSED(ACode)
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
	Q_UNUSED(ASignal)
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
	Q_UNUSED(AType)
	Q_UNUSED(ADeviceId)
	// TODO: implementation
	return true;
}

ISipCall::DeviceState SipCall::deviceState(ISipDevice::Type AType) const
{
	Q_UNUSED(AType)
	// TODO: implementation
	return DS_ENABLED;
}

bool SipCall::setDeviceState(ISipDevice::Type AType, ISipCall::DeviceState AState) const
{
	Q_UNUSED(AType)
	Q_UNUSED(AState)
	// TODO: implementation
	return true;
}

QVariant SipCall::deviceProperty(ISipDevice::Type AType, int AProperty)
{
	Q_UNUSED(AType)
	Q_UNUSED(AProperty)
	// TODO: implementation
	return QVariant();
}

bool SipCall::setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	Q_UNUSED(AType)
	Q_UNUSED(AProperty)
	Q_UNUSED(AValue)
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

SipCall *SipCall::activeCallForId(int id)
{
	return activeCalls.value(id, NULL);
}

void SipCall::onCallState(int call_id, void *e)
{
	Q_UNUSED(call_id)
	Q_UNUSED(e)
	// TODO: implementation
}

void SipCall::onCallMediaState(int call_id)
{
	Q_UNUSED(call_id)
	// TODO: implementation
}

void SipCall::onCallTsxState(int call_id, void *tsx, void *e)
{
	Q_UNUSED(call_id)
	Q_UNUSED(tsx)
	Q_UNUSED(e)
	// TODO: implementation
}

int SipCall::onMyPutFrameCallback(int call_id, void *frame, int w, int h, int stride)
{
	Q_UNUSED(call_id)
	Q_UNUSED(frame)
	Q_UNUSED(w)
	Q_UNUSED(h)
	Q_UNUSED(stride)
	// TODO: implementation
	return 0;
}

int SipCall::onMyPreviewFrameCallback(void *frame, const char *colormodelName, int w, int h, int stride)
{
	Q_UNUSED(frame)
	Q_UNUSED(colormodelName)
	Q_UNUSED(w)
	Q_UNUSED(h)
	Q_UNUSED(stride)
	// TODO: implementation
	return 0;
}
