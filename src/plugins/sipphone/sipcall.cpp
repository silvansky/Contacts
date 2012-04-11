#include "sipcall.h"

#include <QVariant>
#include <QImage>

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
	FSipManager = ASipManager;
	FStanzaProcessor = AStanzaProcessor;

	FSessionId = ASessionId;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	
	FCallId = -1;
	FAccountId = -1;
	FRole = CR_RESPONDER;
	FState = CS_NONE;
	FErrorCode = EC_NONE;

	FSHICallAccept = -1;
	FCallInstances.append(this);

	FRingTimer.setSingleShot(true);
	connect(&FRingTimer,SIGNAL(timeout()),SLOT(onRingTimerTimeout()));

	LogDetail(QString("[SipCall] Call created as RESPONDER sid='%1'").arg(sessionId()));
}

SipCall::SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, const Jid &AStreamJid, const QList<Jid> &ADestinations, const QString &ASessionId)
{
	FSipManager = ASipManager;
	FStanzaProcessor = AStanzaProcessor;

	FSessionId = ASessionId;
	FStreamJid = AStreamJid;
	FDestinations = ADestinations;

	FCallId = -1;
	FAccountId = -1;
	FRole = CR_INITIATOR;
	FState = CS_NONE;
	FErrorCode = EC_NONE;

	FSHICallAccept = -1;
	FCallInstances.append(this);

	LogDetail(QString("[SipCall] Call created as INITIATOR sid='%1'").arg(sessionId()));
}

SipCall::~SipCall()
{
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHICallAccept);
	FCallInstances.removeAll(this);
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
	return FContactJid;
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
			Stanza reques("iq");
			reques.setTo(destination.eFull()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement queryElem = reques.addElement("query",NS_RAMBLER_PHONE);
			queryElem.setAttribute("type","request");
			queryElem.setAttribute("sid",sessionId());
			queryElem.setAttribute("client","deskapp");
			if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,streamJid(),reques,CALL_REQUEST_TIMEOUT))
			{
				LogDetail(QString("[SipCall] Call request sent to '%1', id='%2', sid='%3'").arg(destination.full(),reques.id(),sessionId()));
				FCallRequests.insert(reques.id(),destination);
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
	else if (role()==CR_INITIATOR && state()==CS_NONE)
	{
		setCallState(CS_CALLING);
	}
}

void SipCall::acceptCall()
{
	if (role()==CR_RESPONDER && state()==CS_CALLING)
	{
		setCallState(CS_CONNECTING);
	}
}

void SipCall::rejectCall(ISipCall::RejectionCode ACode)
{
	if (state() == CS_CALLING)
	{
		if (role() == CR_INITIATOR)
		{
			notifyActiveDestinations("cancel");
			setCallState(CS_FINISHED);
		}
		else if (role() == CR_RESPONDER)
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
	}
	else if (state() == CS_CONNECTING)
	{
		// TODO: implementation
	}
	else if (state() == CS_TALKING)
	{
		// TODO: check implementation
		pjsua_call_hangup(FCallId, 0, NULL, NULL);
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
	return FStartCallTime.isValid() ? qAbs(FStartCallTime.secsTo(QDateTime::currentDateTime())) : 0;
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

void SipCall::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid)
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
					setCallError(EC_NOTAVAIL,tr("Call in not supported by destination"));
			}
		}
		else if (role() == CR_INITIATOR)
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
	}

	//char status[80];
	if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
	{
		//		snprintf(status, sizeof(status), "Call is %s (%s)", ci.state_text.ptr, ci.last_status_text.ptr);
		//		showStatus(status);
		//		emit signalCallReleased();
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
	// TODO: new implementation
	pjmedia_frame * _frame = (pjmedia_frame *)frame;
	if(_frame->type == PJMEDIA_FRAME_TYPE_VIDEO)
	{
		int dstSize = w * h * 3;
		unsigned char * dst = new unsigned char[dstSize];

		FC::YUV420PtoRGB32(w, h, stride, (unsigned char *)_frame->buf, dst, dstSize);
		QImage remoteImage((uchar*)dst, w, h, QImage::Format_RGB888);
		// TODO: set image to device
		Q_UNUSED(remoteImage)
	}
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

void SipCall::setCallState(CallState AState)
{
	if (FState != AState)
	{
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
			// TODO: Register on sip server and then call continueAfterRegistration
		}
		else if (AState == CS_TALKING)
		{
			FStartCallTime = QDateTime::currentDateTime();
		}
		FState = AState;
		emit stateChanged(AState);
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
	if (role() == CR_INITIATOR)
	{
		if (ARegistered)
		{
			if (FStanzaProcessor)
			{
				notifyActiveDestinations("accepted");
				Stanza result = FStanzaProcessor->makeReplyResult(FAcceptStanza);
				FStanzaProcessor->sendStanzaOut(streamJid(),result);
			}
			// TODO: Call to responder
		}
		else
		{
			if (FStanzaProcessor)
			{
				notifyActiveDestinations("caller_error");
				ErrorHandler err(ErrorHandler::RECIPIENT_UNAVAILABLE);
				Stanza error = FStanzaProcessor->makeReplyError(FAcceptStanza,err);
				FStanzaProcessor->sendStanzaOut(streamJid(),error);
			}
			setCallError(EC_CONNECTIONERR,tr("Failed to register on SIP server"));
		}
		FActiveDestinations.clear();
	}
	else if (role() == CR_RESPONDER)
	{
		if (FStanzaProcessor)
		{
			Stanza accept("iq");
			accept.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement queryElem = accept.addElement("query",NS_RAMBLER_PHONE);
			queryElem.setAttribute("type","accept");
			queryElem.setAttribute("sid",sessionId());
			if (FStanzaProcessor->sendStanzaRequest(this,streamJid(),accept,CALL_REQUEST_TIMEOUT))
				FCallRequests.insert(accept.id(),contactJid());
			else
				setCallError(EC_CONNECTIONERR,tr("Failed to accept call"));
		}
		// TODO: check implementation
		//pjsua_call_answer(FCallId, PJSIP_SC_OK, NULL, NULL);
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

	if (FSipManager->isRegisteredAtServer(streamJid()))
	{
		pj_status_t status;
		char uriTmp[512];
		const char* uriToCall = AContactJid.full().toUtf8().constData();

		pj_ansi_sprintf(uriTmp, "sip:%s", uriToCall);
		pj_str_t uri = pj_str((char*)uriTmp);

		pj_assert(FCallId == -1);

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

		pjsua_call_id id = FCallId;
		status = pjsua_call_make_call(FAccountId, &uri, &call_setting, NULL, NULL, &id);
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

void SipCall::onRingTimerTimeout()
{
	if (state() == CS_CALLING)
	{
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