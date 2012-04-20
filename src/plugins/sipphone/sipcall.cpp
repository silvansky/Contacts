#include "sipcall.h"

#include <QVariant>
#include <QImage>
#include <QPixmap>

#include <pjsua.h>
#include <assert.h>

#include "pjsipdefines.h"
#include "frameconverter.h"

#include <definitions/namespaces.h>
#include <utils/log.h>

#define RING_TIMEOUT            90000
#define CALL_REQUEST_TIMEOUT    10000

#define SHC_CALL_ACCEPT         "/iq[@type='set']/query[@sid='%1'][@xmlns='" NS_RAMBLER_PHONE "']"

QList<SipCall *> SipCall::FCallInstances;

SipCall::SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId)
{
	init(ASipManager, AStanzaProcessor, AStreamJid, ASessionId);
	FContactJid = AContactJid;
	
	FRole = CR_RESPONDER;

	FRingTimer.setSingleShot(true);
	connect(&FRingTimer,SIGNAL(timeout()),SLOT(onRingTimerTimeout()));

	LogDetail(QString("[SipCall] Call created as RESPONDER sid='%1'").arg(sessionId()));
}

SipCall::SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QList<Jid> &ADestinations, const QString &ASessionId)
{
	init(ASipManager, AStanzaProcessor, AStreamJid, ASessionId);
	FDestinations = ADestinations;

	FRole = CR_INITIATOR;

	LogDetail(QString("[SipCall] Call created as INITIATOR sid='%1'").arg(sessionId()));
}

SipCall::~SipCall()
{
	rejectCall(RC_BYUSER);
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHICallAccept);
	FCallInstances.removeAll(this);
	emit callDestroyed();
	LogDetail(QString("[SipCall] Call destroyed sid='%1'").arg(sessionId()));
}

QObject *SipCall::instance()
{
	return this;
}

Jid SipCall::streamJid() const
{
	return FStreamJid;
}

Jid SipCall::contactJid() const
{
	return FContactJid.isEmpty() ? FDestinations.value(0) : FContactJid;
}

QString SipCall::sessionId() const
{
	return FSessionId;
}

QList<Jid> SipCall::callDestinations() const
{
	return FDestinations;
}

void SipCall::startCall()
{
	if (role()==CR_INITIATOR && state()==CS_NONE)
	{
		foreach(Jid destination, FDestinations)
		{
			Stanza request("iq");
			request.setTo(destination.eFull()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement queryElem = request.addElement("query",NS_RAMBLER_PHONE);
			queryElem.setAttribute("type","request");
			queryElem.setAttribute("sid",sessionId());
			queryElem.setAttribute("client","deskapp");
			if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,streamJid(),request,CALL_REQUEST_TIMEOUT))
			{
				LogDetail(QString("[SipCall] Call request sent to '%1', id='%2', sid='%3'").arg(destination.full(),request.id(),sessionId()));
				FCallRequests.insert(request.id(),destination);
			}
			else
			{
				LogError(QString("[SipCall] Failed to send call request to '%1', sid='%2'").arg(destination.full(),sessionId()));
			}
		}

		if (!FCallRequests.isEmpty())
			setCallState(CS_CALLING);
		else
			setCallError(EC_NOTAVAIL,tr("The destinations is not available"));
	}
	else if (role()==CR_RESPONDER && state()==CS_NONE)
	{
		setCallState(CS_CALLING);
	}
}

void SipCall::acceptCall()
{
	if (role()==CR_RESPONDER)
	{
		if (state() == CS_CALLING)
			setCallState(CS_CONNECTING);
		else if (state() == CS_CONNECTING)
			pjsua_call_answer(FCallId,PJSIP_SC_OK,NULL,NULL);
	}
}

void SipCall::rejectCall(ISipCall::RejectionCode ACode)
{
	switch (state())
	{
	case CS_CALLING:
	{
		if (role() == CR_INITIATOR)
		{
			notifyActiveDestinations("cancel");
			setCallState(CS_FINISHED);
		}
		else //if (role() == CR_RESPONDER)
		{
			if (FStanzaProcessor)
			{
				Stanza deny("iq");
				deny.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = deny.addElement("query",NS_RAMBLER_PHONE);
				if (ACode == RC_BUSY)
					queryElem.setAttribute("type","busy");
				else
					queryElem.setAttribute("type","deny");
				queryElem.setAttribute("sid",sessionId());
				FStanzaProcessor->sendStanzaOut(streamJid(),deny);
			}
			setCallState(CS_FINISHED);
		}
		break;
	}
	case CS_CONNECTING:
	case CS_TALKING:
	{
		// TODO: check implementation
		pj_status_t status = pjsua_call_hangup(FCallId, PJSIP_SC_DECLINE, NULL, NULL);
		if (status == PJ_SUCCESS)
		{
			setCallState(CS_FINISHED);
		}
		else
		{
			LogError(QString("[SipCall::rejectCall]: Failed to end call! pjsua_call_hangup() returned %1").arg(status));
		}
		break;
	}
	default:
		break;
	}
}

ISipCall::CallerRole SipCall::role() const
{
	return FRole;
}

ISipCall::CallState SipCall::state() const
{
	return FState;
}

ISipCall::ErrorCode SipCall::errorCode() const
{
	return FErrorCode;
}

QString SipCall::errorString() const
{
	return FErrorString;
}

quint32 SipCall::callTime() const
{
	if (FCallId != -1)
	{
		pjsua_call_info ci;
		pjsua_call_get_info(FCallId, &ci);
		return ci.connect_duration.sec;
	}
	return 0;
}

QString SipCall::callTimeString() const
{
	QTime t(0, 0, 0, 0);
	if (FCallId != -1)
	{
		pjsua_call_info ci;
		pjsua_call_get_info(FCallId, &ci);
		t.addSecs(ci.connect_duration.sec);
		t.addMSecs(ci.connect_duration.msec);
	}
	return t.toString("hh:mm:ss");
}

bool SipCall::sendDTMFSignal(QChar ASignal)
{
	pj_status_t status;
	pj_str_t digits;
	char digits_tmp[128];

	if (FCallId == -1)
	{
		LogError(QString("[SipCall::sendDTMFSignal]: Failed to send \'%1\' DTMF signal due to inactive call.").arg(ASignal));
		return false;
	}
	QString tmpStr = QString("%1").arg(ASignal);

	pj_ansi_strncpy(digits_tmp, tmpStr.toAscii().constData(), sizeof(digits_tmp));

	digits = pj_str(digits_tmp);
	status = pjsua_call_dial_dtmf(FCallId, &digits);
	if(status != PJ_SUCCESS)
	{
		LogError(QString("[SipCall::sendDTMFSignal]: Failed to send DTMF \'%1\', pjsua_call_dial_dtmf() returned status %2 ").arg(ASignal).arg(status));
	}
	return true;
}

ISipDevice SipCall::activeDevice(ISipDevice::Type AType) const
{
	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
		return camera;
	case ISipDevice::DT_LOCAL_MICROPHONE:
		return microphone;
	case ISipDevice::DT_REMOTE_CAMERA:
		return videoInput;
	case ISipDevice::DT_REMOTE_MICROPHONE:
		return audioOutput;
	default:
		return ISipDevice();
	}
}

bool SipCall::setActiveDevice(ISipDevice::Type AType, int ADeviceId)
{
	if (FSipManager)
	{
		bool deviceFound = true;
		ISipDevice d = FSipManager->getDevice(AType, ADeviceId);
		if ((deviceFound = (d.id != -1)))
		{
			switch (AType)
			{
			case ISipDevice::DT_LOCAL_CAMERA:
				camera = d;
				break;
			case ISipDevice::DT_LOCAL_MICROPHONE:
				microphone = d;
				break;
			case ISipDevice::DT_REMOTE_MICROPHONE:
				audioOutput = d;
				break;
			case ISipDevice::DT_REMOTE_CAMERA:
				videoInput = d;
				break;
			default:
				deviceFound = false;
				break;
			}
			if (deviceFound)
				emit activeDeviceChanged(AType);
		}
		return deviceFound;
	}
	else
		return false;
}

ISipDevice::State SipCall::deviceState(ISipDevice::Type AType) const
{
	Q_UNUSED(AType)
	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
		return cameraState;
		break;
	case ISipDevice::DT_LOCAL_MICROPHONE:
		return microphoneState;
		break;
	case ISipDevice::DT_REMOTE_CAMERA:
		return videoInputState;
		break;
	case ISipDevice::DT_REMOTE_MICROPHONE:
		return audioOutputState;
		break;
	default:
		return ISipDevice::DS_UNAVAIL;
	}
}

bool SipCall::setDeviceState(ISipDevice::Type AType, ISipDevice::State AState)
{
	// TODO: complete implementation
	bool status = false;
	bool stateChanged = false;

	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
	{
		if (AState != cameraState)
		{
			pjsua_call_setting call_setting;
			pjsua_call_setting_default(&call_setting);
			call_setting.vid_cnt = (AState == ISipDevice::DS_ENABLED) ? 1 : 0;
			pj_status_t pjstatus = pjsua_call_reinvite2(FCallId, &call_setting, NULL);
			if ((status = (pjstatus == PJ_SUCCESS)))
			{
				stateChanged = true;
			}
			else
			{
				LogError(QString("[SipCall::setDeviceState]: Camera state changing failed with status %1! State was %2.").arg(pjstatus).arg(AState));
			}

		}
		break;
	}
	case ISipDevice::DT_LOCAL_MICROPHONE:
	{
		break;
	}
	case ISipDevice::DT_REMOTE_CAMERA:
	{
		break;
	}
	case ISipDevice::DT_REMOTE_MICROPHONE:
	{
		break;
	}
	default:
		status = false;
		break;
	}

	if (stateChanged)
		emit deviceStateChanged(AType, AState);

	return status;
}

QVariant SipCall::deviceProperty(ISipDevice::Type AType, int AProperty) const
{
	Q_UNUSED(AType)
	Q_UNUSED(AProperty)
	// TODO: implementation
	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
		return cameraProperties.value(AProperty, QVariant());
		break;
	case ISipDevice::DT_LOCAL_MICROPHONE:
		return microphoneProperties.value(AProperty, QVariant());
		break;
	case ISipDevice::DT_REMOTE_CAMERA:
		return videoInputProperties.value(AProperty, QVariant());
		break;
	case ISipDevice::DT_REMOTE_MICROPHONE:
		return audioOutputProperties.value(AProperty, QVariant());
		break;
	default:
		break;
	}

	return QVariant();
}

bool SipCall::setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	bool propertyChanged = false;
	switch (AType)
	{
	case ISipDevice::DT_LOCAL_CAMERA:
		if (AProperty == ISipDevice::CP_CURRENTFRAME) // readonly property
			return false;
		else
		{
			propertyChanged = (cameraProperties.value(AProperty, QVariant()) != AValue);
			if (propertyChanged)
			{
				cameraProperties.insert(AProperty, AValue);
				// TODO: handle property changes
			}
		}
		break;
	case ISipDevice::DT_LOCAL_MICROPHONE:
		propertyChanged = (microphoneProperties.value(AProperty, QVariant()) != AValue);
		if (propertyChanged)
		{
			microphoneProperties.insert(AProperty, AValue);
			// TODO: handle property changes
		}
		break;
	case ISipDevice::DT_REMOTE_CAMERA:
		if (AProperty == ISipDevice::VP_CURRENTFRAME) // readonly property
			return false;
		else
		{
			propertyChanged = (videoInputProperties.value(AProperty, QVariant()) != AValue);
			if (propertyChanged)
			{
				videoInputProperties.insert(AProperty, AValue);
				// TODO: handle property changes
			}
		}
		break;
	case ISipDevice::DT_REMOTE_MICROPHONE:
		propertyChanged = (audioOutputProperties.value(AProperty, QVariant()) != AValue);
		if (propertyChanged)
		{
			audioOutputProperties.insert(AProperty, AValue);
			// TODO: handle property changes
		}
		break;
	default:
		break;
	}
	if (propertyChanged)
		emit devicePropertyChanged(AType, AProperty, AValue);
	return propertyChanged;
}

void SipCall::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FCallRequests.contains(AStanza.id()))
	{
		Jid destination = FCallRequests.take(AStanza.id());
		if (role() == CR_INITIATOR)
		{
			if (AStanza.type() == "result")
			{
				LogDetail(QString("[SipCall] Call request accepted by '%1', sid='%2").arg(destination.full(),sessionId()));
				FActiveDestinations.append(destination);
			}
			else
			{
				LogDetail(QString("[SipCall] Call request rejected by '%1', sid='%2").arg(destination.full(),sessionId()));
				if (FCallRequests.isEmpty())
					setCallError(EC_NOTAVAIL,tr("Call is not supported by destination"));
			}
		}
		else if (role() == CR_RESPONDER)
		{
			if (AStanza.type() == "result")
			{
				LogDetail(QString("[SipCall] Call accept accepted by '%1', sid='%2").arg(destination.full(),sessionId()));
			}
			else
			{
				LogDetail(QString("[SipCall] Call accept rejected by '%1', sid='%2").arg(destination.full(),sessionId()));
				setCallError(EC_CONNECTIONERR,tr("Remote user failed to register on SIP server"));
			}
		}
	}
}

bool SipCall::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandleId==FSHICallAccept && state()==CS_CALLING)
	{
		bool sendResult = true;
		QString type = AStanza.firstElement("query",NS_RAMBLER_PHONE).attribute("type");
		LogDetail(QString("[SipCall] Action stanza received with type='%1' from='%2', sid='%3'").arg(type,AStanza.from(),sessionId()));
		if (role()==CR_INITIATOR && FActiveDestinations.contains(AStanza.from()))
		{
			AAccept = true;
			if (type == "accept")
			{
				sendResult = false;
				FCallRequests.clear();
				FAcceptStanza = AStanza;
				FContactJid = AStanza.from();
				setCallState(CS_CONNECTING);
			}
			else if (type == "deny")
			{
				FContactJid = AStanza.from();
				notifyActiveDestinations("denied");
				setCallError(EC_REJECTED,tr("Remote user rejected the call"));
			}
			else if (type == "busy")
			{
				FActiveDestinations.removeAll(AStanza.from());
				if (FActiveDestinations.isEmpty())
				{
					FContactJid = AStanza.from();
					setCallError(EC_BUSY,tr("Remote user is busy"));
				}
			}
			else if (type == "timeout_error")
			{
				FActiveDestinations.removeAll(AStanza.from());
				if (FActiveDestinations.isEmpty())
				{
					FContactJid = AStanza.from();
					setCallError(EC_NOANSWER,tr("Remote user is not answering"));
				}
			}
			else if (type == "callee_error")
			{
				FContactJid = AStanza.from();
				notifyActiveDestinations("callee_error");
				setCallError(EC_CONNECTIONERR,tr("Remote user failed to register on SIP server"));
			}
		}
		else if (role()==CR_RESPONDER && contactJid()==AStanza.from())
		{
			AAccept = true;
			if (type == "accepted")
			{
				setCallState(CS_FINISHED); // Accepted by another resource
			}
			else if (type == "cancel")
			{
				setCallError(EC_REJECTED,tr("Remote user rejected the call"));
			}
			else if (type == "denied")
			{
				setCallState(CS_FINISHED); // Rejected by another resource
			}
			else if (type == "caller_error")
			{
				setCallError(EC_CONNECTIONERR,tr("Remote user failed to register on SIP server"));
			}
			else if (type == "callee_error")
			{
				setCallError(EC_CONNECTIONERR,tr("Another resource failed to register on SIP server"));
			}
		}

		if (AAccept && sendResult)
		{
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	return false;
}

int SipCall::callId() const
{
	return FCallId;
}

int SipCall::accountId() const
{
	return FAccountId;
}

void SipCall::setCallParams(int AAccountId, int ACallId)
{
	FCallId = ACallId;
	FAccountId = AAccountId;
}

SipCall *SipCall::findCallById(int ACallId)
{
	foreach(SipCall *call, FCallInstances)
		if (call->callId() == ACallId)
			return call;
	return NULL;
}

void SipCall::onCallState(int call_id, void *e)
{
	Q_UNUSED(e)
	// TODO: new implementation
	pjsua_call_info ci;

	pjsua_call_get_info(call_id, &ci);

	if ( FCallId == -1 && ci.state < PJSIP_INV_STATE_DISCONNECTED && ci.state != PJSIP_INV_STATE_INCOMING)
	{
		//		emit signalNewCall(call_id, false);
	}


	if(ci.state == PJSIP_INV_STATE_CONFIRMED)
	{
		//emit signalShowSipPhoneWidget(NULL);
		setCallState(CS_TALKING);
	}

	//char status[80];
	if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
	{
		//		snprintf(status, sizeof(status), "Call is %s (%s)", ci.state_text.ptr, ci.last_status_text.ptr);
		//		showStatus(status);
		//		emit signalCallReleased();
		setCallState(CS_FINISHED);
	}
	else
	{
		//		snprintf(status, sizeof(status), "Call is %s", pjsip_inv_state_name(ci.state));
		//		showStatus(status);
	}
}

void SipCall::onCallMediaState(int call_id)
{
	Q_UNUSED(call_id)
	// TODO: new implementation
	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);

	for (unsigned i=0; i<ci.media_cnt; ++i)
	{
		if (ci.media[i].type == PJMEDIA_TYPE_AUDIO)
		{
			switch (ci.media[i].status)
			{
			case PJSUA_CALL_MEDIA_ACTIVE:
				pjsua_conf_connect(ci.media[i].stream.aud.conf_slot, 0);
				pjsua_conf_connect(0, ci.media[i].stream.aud.conf_slot);
				break;
			default:
				break;
			}
		}
		else if (ci.media[i].type == PJMEDIA_TYPE_VIDEO)
		{
			//			emit signalInitVideoWindow();
		}
	}
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
	// TODO: check implementation
	pjmedia_frame * _frame = (pjmedia_frame *)frame;
	if(_frame->type == PJMEDIA_FRAME_TYPE_VIDEO)
	{
		int dstSize = w * h * 3;
		unsigned char * dst = new unsigned char[dstSize];

		FC::YUV420PtoRGB32(w, h, stride, (unsigned char *)_frame->buf, dst, dstSize);
		QImage remoteImage((uchar*)dst, w, h, QImage::Format_RGB888);

		QPixmap remotePixmap = QPixmap::fromImage(remoteImage);
		videoInputProperties[ISipDevice::VP_CURRENTFRAME] = remotePixmap;
		emit devicePropertyChanged(ISipDevice::DT_REMOTE_CAMERA, ISipDevice::VP_CURRENTFRAME, remotePixmap);
	}
	return 0;
}

int SipCall::onMyPreviewFrameCallback(void *frame, const char *colormodelName, int w, int h, int stride)
{
	pjmedia_frame * pjframe = (pjmedia_frame*)frame;
	// TODO: check implementation
	if (pjframe->type == PJMEDIA_FRAME_TYPE_VIDEO)
	{
		int dstSize = w * h * 3;
		unsigned char * dst = new unsigned char[dstSize];

		QImage previewImage;

		if(strstr(colormodelName, "RGB") != 0)
		{
			memcpy(dst, pjframe->buf, dstSize);
			previewImage = QImage((uchar*)dst, w, h, QImage::Format_RGB888).rgbSwapped();
		}
		else if(strstr(colormodelName, "YUY") != 0)
		{
			FC::YUYV422PtoRGB32(w, h, (unsigned char *)pjframe->buf, dst, dstSize);
			previewImage = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();
		}
		else
		{
			FC::YUV420PtoRGB32(w, h, stride, (unsigned char *)pjframe->buf, dst, dstSize);
			previewImage = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();
		}
		delete[] dst;

		QPixmap previewPixmap = QPixmap::fromImage(previewImage);
		cameraProperties[ISipDevice::CP_CURRENTFRAME] = previewPixmap;
		emit devicePropertyChanged(ISipDevice::DT_LOCAL_CAMERA, ISipDevice::CP_CURRENTFRAME, previewPixmap);
	}
	return 0;
}

void SipCall::init(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QString &ASessionId)
{
	FSipManager = AManager;
	FStanzaProcessor = AStanzaProcessor;

	FSessionId = ASessionId;
	FStreamJid = AStreamJid;
	FCallId = -1;
	FAccountId = -1;
	FSHICallAccept = -1;
	FState = CS_NONE;
	FErrorCode = EC_NONE;

	connect(FSipManager->instance(), SIGNAL(registeredAtServer(const Jid &)), SLOT(onRegisteredAtServer(const Jid &)));
	connect(FSipManager->instance(), SIGNAL(unregisteredAtServer(const Jid &)), SLOT(onUnRegisteredAtServer(const Jid &)));
	connect(FSipManager->instance(), SIGNAL(registrationAtServerFailed(const Jid &)), SLOT(onRegistraitionAtServerFailed(const Jid &)));

	FCallInstances.append(this);
}

void SipCall::setCallState(CallState AState)
{
	if (FState != AState)
	{
		FState = AState;
		LogDetail(QString("[SipCall] Call state changed to '%1', sid='%2'").arg(AState).arg(sessionId()));
		if (AState == CS_CALLING)
		{
			IStanzaHandle handle;
			handle.handler = this;
			handle.order = SHO_DEFAULT;
			handle.streamJid = streamJid();
			handle.direction = IStanzaHandle::DirectionIn;
			handle.conditions.append(QString(SHC_CALL_ACCEPT).arg(sessionId()));
			FSHICallAccept = FStanzaProcessor->insertStanzaHandle(handle);
			FRingTimer.start(RING_TIMEOUT);
		}
		else if (AState == CS_CONNECTING)
		{
			if (FSipManager)
			{
				if (!FSipManager->isRegisteredAtServer(FStreamJid))
					FSipManager->registerAtServer(FStreamJid);
				else
					continueAfterRegistration(true);
			}
		}
		emit stateChanged(FState);
	}
}

void SipCall::setCallError(ErrorCode ACode, const QString &AMessage)
{
	if (FErrorCode == EC_NONE)
	{
		LogDetail(QString("[SipCall] Call error changed to '%1' with message='%2', sid='%3'").arg(ACode).arg(AMessage).arg(sessionId()));
		FErrorCode = ACode;
		FErrorString = AMessage;
		setCallState(CS_ERROR);
	}
}

void SipCall::continueAfterRegistration(bool ARegistered)
{
	// TODO: check/test code
	if (role() == CR_INITIATOR)
	{
		if (FStanzaProcessor)
		{
			if (ARegistered)
			{
				notifyActiveDestinations("accepted");
				Stanza result = FStanzaProcessor->makeReplyResult(FAcceptStanza);
				FStanzaProcessor->sendStanzaOut(streamJid(), result);
				sipCallTo(FContactJid);
			}
			else
			{
				notifyActiveDestinations("caller_error");
				ErrorHandler err(ErrorHandler::RECIPIENT_UNAVAILABLE);
				Stanza error = FStanzaProcessor->makeReplyError(FAcceptStanza,err);
				FStanzaProcessor->sendStanzaOut(streamJid(), error);
				setCallError(EC_CONNECTIONERR,tr("Failed to register on SIP server"));
			}
		}
		FActiveDestinations.clear();
	}
	else if (role() == CR_RESPONDER)
	{
		if (FStanzaProcessor)
		{
			if (ARegistered)
			{
				Stanza accept("iq");
				accept.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = accept.addElement("query", NS_RAMBLER_PHONE);
				queryElem.setAttribute("type", "accept");
				queryElem.setAttribute("sid", sessionId());
				if (FStanzaProcessor->sendStanzaRequest(this, streamJid(), accept, CALL_REQUEST_TIMEOUT))
					FCallRequests.insert(accept.id(), contactJid());
				else
					setCallError(EC_CONNECTIONERR, tr("Failed to accept call"));
			}
			else
			{
				Stanza error("iq");
				error.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = error.addElement("query", NS_RAMBLER_PHONE);
				queryElem.setAttribute("type", "callee_error");
				queryElem.setAttribute("sid", sessionId());
				setCallError(EC_CONNECTIONERR,tr("Failed to register on SIP server"));
				FStanzaProcessor->sendStanzaOut(streamJid(),error);
			}
		}
	}
}

void SipCall::notifyActiveDestinations(const QString &AType)
{
	foreach(Jid destination, FActiveDestinations)
	{
		if (destination != FContactJid)
		{
			Stanza reply("iq");
			reply.setTo(destination.eFull()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement queryElem = reply.addElement("query",NS_RAMBLER_PHONE);
			queryElem.setAttribute("type",AType);
			queryElem.setAttribute("sid",sessionId());
			FStanzaProcessor->sendStanzaOut(streamJid(),reply);
		}
	}
}

void SipCall::sipCallTo(const Jid &AContactJid)
{
	// TODO: implementation
	// step 1: check registration at SIP server
	// step 2: ask SIP server to connect
	// step 3:

	if (FSipManager && FSipManager->isRegisteredAtServer(streamJid()))
	{
		pj_status_t status;
		char uriTmp[512];
		const char* uriToCall = AContactJid.eBare().toAscii().constData();

		pj_ansi_sprintf(uriTmp, "sip:%s", uriToCall);
		pj_str_t uri = pj_str((char*)uriTmp);

		if (FCallId == -1)
		{
			pjsua_call_setting call_setting;
			pjsua_call_setting_default(&call_setting);

			if (AContactJid.domain() == PHONE_TRANSPORT_DOMAIN)
			{
				call_setting.vid_cnt = 0;
			}
			else
			{
#if defined(HAS_VIDEO_SUPPORT)
				call_setting.vid_cnt = HAS_VIDEO_SUPPORT;
#else
				call_setting.vid_cnt = 0;
#endif
				call_setting.aud_cnt = 1;
			}

			pjsua_call_id id = -1;
			status = pjsua_call_make_call(FAccountId, &uri, &call_setting, NULL, NULL, &id);
			FCallId = id;
			if (status != PJ_SUCCESS)
			{
				LogError(QString("[SipCall::sipCallTo]: pjsua_call_make_call() returned status %1, uri is \'%2\'").arg(status).arg(uriTmp));
				setCallError(ISipCall::EC_CONNECTIONERR, QString("SIP call to %1 failed").arg(AContactJid.full()));
				if (status == PJMEDIA_EAUD_NODEFDEV)
				{
					LogError(QString("[SipCall::sipCallTo]: Default device not found!"));
					//emit signal_DeviceError();
					//				emit signal_InviteStatus(false, 2, tr("Failed to find default audio device"));
				}
				//			showError("make call", status);
				//return;
			}
			//		emit signal_InviteStatus(true, 0, "");
		}
		else
		{
			LogError(QString("[SipCall::sipCallTo]: Call is already active with stream jid %1 and contact %2").arg(streamJid().full(), contactJid().full()));
		}
	}
	else
	{
		LogError(QString("[SipCall::sipCallTo]: Stream jid %1 isn\'t registered at server!").arg(streamJid().full()));
	}
}

void SipCall::onRingTimerTimeout()
{
	if (state() == CS_CALLING)
	{
		LogDetail(QString("[SipCall] Ring timer timed out, sid='%1'").arg(sessionId()));
		if (role() == CR_INITIATOR)
		{
			notifyActiveDestinations("timeout_error");
			setCallError(EC_NOANSWER,tr("Remote user is not answering"));
		}
		else if (role() == CR_INITIATOR)
		{
			if (FStanzaProcessor)
			{
				Stanza timeout("iq");
				timeout.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = timeout.addElement("query",NS_RAMBLER_PHONE);
				queryElem.setAttribute("type","timeout_error");
				queryElem.setAttribute("sid",sessionId());
				FStanzaProcessor->sendStanzaOut(streamJid(),timeout);
			}
			setCallError(EC_NOANSWER,tr("Call was not accepted too long"));
		}
	}
}

void SipCall::onRegisteredAtServer(const Jid &AStreamJid)
{
	Q_UNUSED(AStreamJid)
	// TODO: check implementation
	if (state() == CS_CONNECTING)
	{
		LogDetail(QString("[SipCall] Successfully registered at SIP server, sid='%1'").arg(sessionId()));
		continueAfterRegistration(true);
	}
}

void SipCall::onUnRegisteredAtServer(const Jid &AStreamJid)
{
	Q_UNUSED(AStreamJid)
	// TODO: implementation
}

void SipCall::onRegistraitionAtServerFailed(const Jid &AStreamJid)
{
	Q_UNUSED(AStreamJid)
	// TODO: check implementation
	if (state() == CS_CONNECTING)
	{
		LogError(QString("[SipCall] Failed to register at SIP server, sid='%1'").arg(sessionId()));
		continueAfterRegistration(false);
	}
}
