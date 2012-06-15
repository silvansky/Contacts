#include "sipcall.h"

#include <QVariant>
#include <QImage>
#include <QPixmap>

#include <pjsua.h>
#include <assert.h>

#include "pjsipdefines.h"
#include "frameconverter.h"
#include "sipmanager.h"

#include <definitions/namespaces.h>
#include <utils/log.h>

#define RING_TIMEOUT            90000
#define CALL_REQUEST_TIMEOUT    10000
#define DEF_VOLUME              1.0f
#define MAX_VOLUME              4.0f

#define SHC_CALL_ACCEPT         "/iq[@type='set']/query[@sid='%1'][@xmlns='" NS_RAMBLER_PHONE "']"
#define SHC_DEVICE_STATES       "/message/x[@sid='%1'][@xmlns='" NS_RAMBLER_PHONE_DEVICESTATES "']"

QList<SipCall *> SipCall::FCallInstances;


SipCall::SipCall(ISipManager *ASipManager, IXmppStream *AXmppStream, const QString &APhoneNumber, const QString &ASessionId)
{
	FDirectCall = true;
	FRole = CR_INITIATOR;
	FContactJid = Jid(APhoneNumber,"vsip.rambler.ru",QString::null);
	init(ASipManager, NULL, AXmppStream, ASessionId);

	LogDetail(QString("[SipCall] Call created as INITIATOR FOR DIRECT CALL, sid='%1'").arg(sessionId()));
}

SipCall::SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const Jid &AContactJid, const QString &ASessionId)
{
	FDirectCall = false;
	FRole = CR_RESPONDER;
	FContactJid = AContactJid;
	init(ASipManager, AStanzaProcessor, AXmppStream, ASessionId);

	LogDetail(QString("[SipCall] Call created as RESPONDER, sid='%1'").arg(sessionId()));
}

SipCall::SipCall(ISipManager *ASipManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const QList<Jid> &ADestinations, const QString &ASessionId)
{
	FDirectCall = false;
	FRole = CR_INITIATOR;
	FDestinations = ADestinations;
	init(ASipManager, AStanzaProcessor, AXmppStream, ASessionId);

	LogDetail(QString("[SipCall] Call created as INITIATOR, sid='%1'").arg(sessionId()));
}

SipCall::~SipCall()
{
	rejectCall(RC_BYUSER);
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHICallAccept);
	FCallInstances.removeAll(this);
	//if (findCalls(streamJid()).isEmpty())
	//	FSipManager->setSipAccountRegistration(streamJid(),false);
	emit callDestroyed();
	LogDetail(QString("[SipCall] Call destroyed, sid='%1'").arg(sessionId()));
}

QObject *SipCall::instance()
{
	return this;
}

bool SipCall::isDirectCall() const
{
	return FDirectCall;
}

Jid SipCall::streamJid() const
{
	return FXmppStream->streamJid();
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
	if ((role() == CR_INITIATOR) && (state() == CS_INIT))
	{
		if (isDirectCall())
		{
			setCallState(CS_CONNECTING);
		}
		else if (!FDestinations.isEmpty())
		{
			if (FStanzaProcessor)
			{
				foreach(Jid destination, FDestinations)
				{
					Stanza request("iq");
					request.setTo(destination.eFull()).setType("set").setId(FStanzaProcessor->newId());
					QDomElement queryElem = request.addElement("query", NS_RAMBLER_PHONE);
					queryElem.setAttribute("type", "request");
					queryElem.setAttribute("sid", sessionId());
					queryElem.setAttribute("client", "deskapp");
					if (FStanzaProcessor->sendStanzaRequest(this, streamJid(), request, CALL_REQUEST_TIMEOUT))
					{
						LogDetail(QString("[SipCall] Call request sent to '%1', id='%2', sid='%3'").arg(destination.full(), request.id(), sessionId()));
						FCallRequests.insert(request.id(), destination);
					}
					else
					{
						LogError(QString("[SipCall] Failed to send call request to '%1', sid='%2'").arg(destination.full(), sessionId()));
					}
				}
			}

			if (!FCallRequests.isEmpty())
				setCallState(CS_CALLING);
			else
				setCallError(EC_NOTAVAIL);
		}
		else
		{
			LogError(QString("[SipCall] Call destination is not specified, sid=%1").arg(sessionId()));
			setCallError(EC_NOTAVAIL);
		}
	}
	else if ((role() == CR_RESPONDER) && (state() == CS_INIT))
	{
		setCallState(CS_CALLING);
	}
}

void SipCall::acceptCall()
{
	if (role() == CR_RESPONDER)
	{
		if (state() == CS_CALLING)
			setCallState(CS_CONNECTING);
		else if (state() == CS_CONNECTING)
			pjsua_call_answer(FCallId, PJSIP_SC_OK, NULL, NULL);
	}
}

void SipCall::rejectCall(ISipCall::RejectionCode ACode)
{
	switch (state())
	{
	case CS_INIT:
		{
			FRejectCode = ACode;
			QTimer::singleShot(0,this,SLOT(onDelayedRejection()));
			break;
		}
	case CS_CALLING:
		{
			FRejectCode = ACode;
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
			break;
		}
	case CS_CONNECTING:
	case CS_TALKING:
		{
			FRejectCode = ACode;
			pj_status_t status = (FCallId != -1) ? pjsua_call_hangup(FCallId, PJSIP_SC_DECLINE, NULL, NULL) : PJ_SUCCESS;
			if (status != PJ_SUCCESS)
				LogError(QString("[SipCall::rejectCall]: Failed to end call! pjsua_call_hangup() returned (%1) %2").arg(status).arg(SipManager::resolveErrorCode(status)));
			setCallState(CS_FINISHED);
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
	switch (errorCode())
	{
	case EC_EMPTY:
		return QString::null;
	case EC_BUSY:
		return tr("Remote user is now talking");
	case EC_NOTAVAIL:
		return tr("Remote user could not accept the call");
	case EC_NOANSWER:
		return tr("Remote user did not accept the call");
	case EC_REJECTED:
		return tr("Remote user rejected the call");
	case EC_CONNECTIONERR:
		return tr("Connection error");
	default:
		return tr("Undefined error");
	}
}

ISipCall::RejectionCode SipCall::rejectCode() const
{
	return FRejectCode;
}

quint32 SipCall::callTime() const
{
	if (!FStartCallTime.isNull())
		return FStartCallTime.msecsTo(FStopCallTime.isNull() ? QDateTime::currentDateTime() : FStopCallTime);
	return 0;
}

QString SipCall::callTimeString() const
{
	QTime time = QTime(0, 0, 0, 0).addMSecs(callTime());
	return time.toString(time.hour()>0 ? "hh:mm:ss" : "mm:ss");
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
		LogError(QString("[SipCall::sendDTMFSignal]: Failed to send DTMF \'%1\', pjsua_call_dial_dtmf() returned status (%2) %3").arg(ASignal).arg(status).arg(SipManager::resolveErrorCode(status)));
	}
	return true;
}

ISipDevice::State SipCall::deviceState(ISipDevice::Type AType) const
{
	return FDeviceStates.value(AType,ISipDevice::DS_UNAVAIL);
}

bool SipCall::setDeviceState(ISipDevice::Type AType, ISipDevice::State AState)
{
	if (FCallId == -1)
	{
		LogError(QString("[SipCall::setDeviceState]: Can\'t change diveces state for inactive call!"));
		return false;
	}
	else if (AState == ISipDevice::DS_UNAVAIL)
	{
		LogError(QString("[SipCall::setDeviceState]: Can\'t change diveces state to ISipDevice::DS_UNAVAIL!"));
		return false;
	}
	else if (deviceState(AType) != AState)
	{
		pj_status_t pjstatus = -1;
		if (AType == ISipDevice::DT_LOCAL_CAMERA)
		{
			if (AState == ISipDevice::DS_ENABLED)
				pjstatus = pjsua_call_set_vid_strm(FCallId, PJSUA_CALL_VID_STRM_START_TRANSMIT, NULL);
			else
				pjstatus = pjsua_call_set_vid_strm(FCallId, PJSUA_CALL_VID_STRM_STOP_TRANSMIT, NULL);
		}
		else if (AType == ISipDevice::DT_REMOTE_CAMERA)
		{
			pjsua_call_setting call_setting;
			pjsua_call_setting_default(&call_setting);
			call_setting.vid_cnt = (AState == ISipDevice::DS_ENABLED) ? 1 : 0;
			pjstatus = pjsua_call_reinvite2(FCallId, &call_setting, NULL);
		}
		else if (AType == ISipDevice::DT_LOCAL_MICROPHONE)
		{
			pjstatus = pjsua_aud_stream_pause_state_change(FCallId, PJMEDIA_DIR_CAPTURE, (AState == ISipDevice::DS_ENABLED) ? false : true);
		}
		else if (AType == ISipDevice::DT_REMOTE_MICROPHONE)
		{
			pjstatus = pjsua_aud_stream_pause_state_change(FCallId, PJMEDIA_DIR_PLAYBACK, (AState == ISipDevice::DS_ENABLED) ? false : true);
		}
		if (pjstatus == PJ_SUCCESS)
		{
			updateDeviceStates();
			return true;
		}
		else
		{
			LogError(QString("[SipCall::setDeviceState]: Failed to change device(type=%1) state to %2, status=%3, error='%4'").arg(AType).arg(AState).arg(pjstatus).arg(SipManager::resolveErrorCode(pjstatus)));
			return false;
		}
	}
	return true;
}

QVariant SipCall::deviceProperty(ISipDevice::Type AType, int AProperty) const
{
	return FDeviceProperties.value(AType).value(AProperty);
}

bool SipCall::setDeviceProperty(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	if (deviceProperty(AType,AProperty) != AValue)
	{
		int changed = false;
		if (AType == ISipDevice::DT_LOCAL_CAMERA)
		{
			switch (AProperty)
			{
			case ISipDevice::LCP_CURRENTFRAME:
			case ISipDevice::LCP_AVAIL_RESOLUTIONS:
			case ISipDevice::LCP_RESOLUTION:
			case ISipDevice::LCP_BRIGHTNESS:
				break;
			default:
				changed = true;
			}
		}
		else if (AType == ISipDevice::DT_REMOTE_CAMERA)
		{
			switch (AProperty)
			{
			case ISipDevice::RCP_CURRENTFRAME:
				break;
			default:
				changed = true;
			}
		}
		else if (AType == ISipDevice::DT_LOCAL_MICROPHONE)
		{
			switch (AProperty)
			{
			case ISipDevice::LMP_VOLUME:
				{
					if (FCallId != -1)
					{
						pjsua_call_info ci;
						pjsua_call_get_info(FCallId, &ci);
						pj_status_t pjstatus = pjsua_conf_adjust_tx_level(ci.conf_slot, AValue.toFloat());
						if (pjstatus == PJ_SUCCESS)
							changed = true;
						else
							LogError(QString("[SipCall::setDeviceProperty]: Failed to change device(type=%1) property=%2 to value=%3, pjstatus=%4, pjerror='%5'").arg(AType).arg(AProperty).arg(AValue.toFloat()).arg(pjstatus).arg(SipManager::resolveErrorCode(pjstatus)));
					}
				}
				break;
			case ISipDevice::LMP_MAX_VOLUME:
				break;
			default:
				changed= true;
			}
		}
		else if (AType == ISipDevice::DT_REMOTE_MICROPHONE)
		{
			switch (AProperty)
			{
			case ISipDevice::RMP_VOLUME:
				{
					if (FCallId != -1)
					{
						pjsua_call_info ci;
						pjsua_call_get_info(FCallId, &ci);
						pj_status_t pjstatus = pjsua_conf_adjust_rx_level(ci.conf_slot, AValue.toFloat());
						if (pjstatus == PJ_SUCCESS)
							changed = true;
						else
							LogError(QString("[SipCall::setDeviceProperty]: Failed to change device(%1) property(%2) to %3, pjstatus=%4, pjerror='%5'").arg(AType).arg(AProperty).arg(AValue.toFloat()).arg(pjstatus).arg(SipManager::resolveErrorCode(pjstatus)));
					}
				}
				break;
			case ISipDevice::RMP_MAX_VOLUME:
				break;
			default:
				changed = true;
			}
		}
		if (changed)
			changeDeviceProperty(AType,AProperty,AValue);
		return changed;
	}
	return true;
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
				LogError(QString("[SipCall] Call request rejected by '%1', sid='%2").arg(destination.full(),sessionId()));
				if (FCallRequests.isEmpty() && FActiveDestinations.isEmpty())
					setCallError(EC_NOTAVAIL);
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
				LogError(QString("[SipCall] Call accept rejected by '%1', sid='%2").arg(destination.full(),sessionId()));
				setCallError(EC_CONNECTIONERR);
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
				setCallError(EC_REJECTED);
			}
			else if (type == "busy")
			{
				FActiveDestinations.removeAll(AStanza.from());
				if (FActiveDestinations.isEmpty())
				{
					FContactJid = AStanza.from();
					setCallError(EC_BUSY);
				}
			}
			else if (type == "timeout_error")
			{
				FActiveDestinations.removeAll(AStanza.from());
				if (FActiveDestinations.isEmpty())
				{
					FContactJid = AStanza.from();
					setCallError(EC_NOANSWER);
				}
			}
			else if (type == "callee_error")
			{
				FContactJid = AStanza.from();
				notifyActiveDestinations("callee_error");
				setCallError(EC_CONNECTIONERR);
			}
		}
		else if (role()==CR_RESPONDER && contactJid()==AStanza.from())
		{
			AAccept = true;
			if (type == "accepted")
			{
				setCallState(CS_FINISHED); // Accepted by another resource
			}
			else if (type == "cancel" || type == "timeout_error")
			{
				setCallError(EC_REJECTED);
			}
			else if (type == "denied")
			{
				FRejectCode = RC_BYUSER;
				setCallState(CS_FINISHED); // Rejected by another resource
			}
			else if (type == "caller_error")
			{
				setCallError(EC_CONNECTIONERR);
			}
			else if (type == "callee_error")
			{
				setCallError(EC_CONNECTIONERR);
			}
		}

		if (AAccept && sendResult)
		{
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (AHandleId==FSHIDeviceStates && (state()==CS_CONNECTING || state()==CS_TALKING))
	{
		AAccept = true;
		LogDetail(QString("[SipCall] Device states update received from='%2', sid='%3'").arg(AStanza.from(),sessionId()));

		QDomElement deviceElem = AStanza.firstElement("x",NS_RAMBLER_PHONE_DEVICESTATES).firstChildElement();
		while (!deviceElem.isNull())
		{
			ISipDevice::Type type = ISipDevice::DT_UNDEFINED;
			if (deviceElem.tagName() == "camera")
				type = ISipDevice::DT_REMOTE_CAMERA;
			else if (deviceElem.tagName() == "microphone")
				type = ISipDevice::DT_REMOTE_MICROPHONE;

			if (type != ISipDevice::DT_UNDEFINED)
			{
				QString text = deviceElem.text();
				if (text=="enabled" && deviceState(type)!=ISipDevice::DS_ENABLED)
					changeDeviceState(type,ISipDevice::DS_ENABLED);
				else if (text=="disabled" && deviceState(type)!=ISipDevice::DS_DISABLED)
					changeDeviceState(type,ISipDevice::DS_DISABLED);
				else if (text=="unavail" && deviceState(type)!=ISipDevice::DS_UNAVAIL)
					changeDeviceState(type,ISipDevice::DS_UNAVAIL);
			}
			deviceElem = deviceElem.nextSiblingElement();
		}
	}
	return false;
}

int SipCall::callId() const
{
	return FCallId;
}

bool SipCall::acceptIncomingCall(int ACallId)
{
	if (role() == CR_RESPONDER)
	{
		if (state() == CS_CONNECTING)
		{
			FCallId = ACallId;
			pjsua_call_answer(FCallId, PJSIP_SC_OK, NULL, NULL);
			return true;
		}
	}
	return false;
}

void SipCall::onCallState(int call_id, void *e)
{
	Q_UNUSED(e);
	// NOTE: This function calls from not main gui thread
	if (FCallId == call_id)
	{
		pjsua_call_info ci;
		pjsua_call_get_info(call_id, &ci);

		if(ci.state == PJSIP_INV_STATE_CONFIRMED)
		{
			emit startUpdateCallState(CS_TALKING);
		}
		else if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
		{
			emit startUpdateCallState(CS_FINISHED);
		}
	}
}

void SipCall::onCallMediaState(int call_id)
{
	// NOTE: This function calls from not main gui thread
	if (FCallId == call_id)
	{
		pjsua_call_info ci;
		pjsua_call_get_info(call_id, &ci);

		for (unsigned i=0; i<ci.media_cnt; ++i)
		{
			if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && ci.media[i].status == PJSUA_CALL_MEDIA_ACTIVE)
			{
				pjsua_conf_connect(ci.media[i].stream.aud.conf_slot,0);
				pjsua_conf_connect(0,ci.media[i].stream.aud.conf_slot);
				break;
			}
		}
		emit startUpdateDeviceStates();
	}
}

void SipCall::onCallTsxState(int call_id, void *tsx, void *e)
{
	Q_UNUSED(call_id)
	Q_UNUSED(tsx)
	Q_UNUSED(e)
	// TODO: implementation
}

int SipCall::onMyPutFrameCallback(void *frame, int w, int h, int stride)
{
	pjmedia_frame * _frame = (pjmedia_frame *)frame;
	if(_frame->type == PJMEDIA_FRAME_TYPE_VIDEO)
	{
		int dstSize = w * h * 3;
		unsigned char * dst = new unsigned char[dstSize];

		FC::YUV420PtoRGB32(w, h, stride, (unsigned char *)_frame->buf, dst, dstSize);
		QImage remoteImage = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();
		delete [] dst;

		emit startUpdateDeviceProperty(ISipDevice::DT_REMOTE_CAMERA,ISipDevice::RCP_CURRENTFRAME,remoteImage);
	}
	return 0;
}

int SipCall::onMyPreviewFrameCallback(void *frame, const char *colormodelName, int w, int h, int stride)
{
	pjmedia_frame * pjframe = (pjmedia_frame*)frame;
	if (pjframe->type == PJMEDIA_FRAME_TYPE_VIDEO && pjframe->buf != NULL && pjframe->size > 0)
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

		emit startUpdateDeviceProperty(ISipDevice::DT_LOCAL_CAMERA, ISipDevice::LCP_CURRENTFRAME,previewImage);
	}
	return 0;
}

SipCall *SipCall::findCallById(int ACallId)
{
	foreach(SipCall *call, FCallInstances)
		if (call->callId() == ACallId)
			return call;
	return NULL;
}

QList<ISipCall*> SipCall::findCalls( const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId)
{
	QList<ISipCall*> found;
	foreach (ISipCall *call, FCallInstances)
	{
		if (AStreamJid.isEmpty() || (call->streamJid() && AStreamJid))
		{
			if (AContactJid.isEmpty() || (call->contactJid() && AContactJid))
			{
				if (ASessionId.isEmpty() || call->sessionId()==ASessionId)
				{
					found << call;
				}
			}
		}
	}
	return found;
}

void SipCall::init(ISipManager *AManager, IStanzaProcessor *AStanzaProcessor, IXmppStream *AXmppStream, const QString &ASessionId)
{
	FSipManager = AManager;
	FXmppStream = AXmppStream;
	FStanzaProcessor = AStanzaProcessor;

	FSessionId = ASessionId;
	FCallId = -1;
	FAccountId = -1;
	FSHICallAccept = -1;
	FSHIDeviceStates = -1;
	FState = CS_INIT;
	FErrorCode = EC_EMPTY;
	FRejectCode = RC_EMPTY;

	changeDeviceProperty(ISipDevice::DT_LOCAL_MICROPHONE,ISipDevice::LMP_MAX_VOLUME,MAX_VOLUME);
	changeDeviceProperty(ISipDevice::DT_LOCAL_MICROPHONE,ISipDevice::LMP_VOLUME,DEF_VOLUME);

	changeDeviceProperty(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::RMP_MAX_VOLUME,MAX_VOLUME);
	changeDeviceProperty(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::RMP_VOLUME,DEF_VOLUME);

	FRingTimer.setSingleShot(true);
	connect(&FRingTimer, SIGNAL(timeout()), SLOT(onRingTimerTimeout()));

	connect(FSipManager->instance(), SIGNAL(sipAccountRegistrationChanged(int,bool)), SLOT(onSipAccountRegistrationChanged(int, bool)));

	connect(this,SIGNAL(startUpdateDeviceStates()),SLOT(updateDeviceStates()),Qt::QueuedConnection);
	connect(this,SIGNAL(startUpdateCallState(int)),SLOT(updateCallState(int)),Qt::QueuedConnection);
	connect(this,SIGNAL(startUpdateDeviceProperty(int,int,const QVariant &)),SLOT(updateDeviceProperty(int,int,const QVariant &)),Qt::QueuedConnection);

	FCallInstances.append(this);
}

void SipCall::sendLocalDeviceStates() const
{
	if (FStanzaProcessor && !isDirectCall())
	{
		Stanza update("message");
		update.setTo(contactJid().eFull());
		QDomElement xElem = update.addElement("x",NS_RAMBLER_PHONE_DEVICESTATES);
		xElem.setAttribute("sid",sessionId());
		
		foreach(ISipDevice::Type type, QList<ISipDevice::Type>()<<ISipDevice::DT_LOCAL_CAMERA<<ISipDevice::DT_LOCAL_MICROPHONE)
		{
			QDomElement deviceElem;
			if (type == ISipDevice::DT_LOCAL_CAMERA)
				deviceElem = xElem.appendChild(update.createElement("camera")).toElement();
			else if (type == ISipDevice::DT_LOCAL_MICROPHONE)
				deviceElem = xElem.appendChild(update.createElement("microphone")).toElement();
			if (!deviceElem.isNull())
			{
				ISipDevice::State state = deviceState(type);
				if (state == ISipDevice::DS_ENABLED)
					deviceElem.appendChild(update.createTextNode("enabled"));
				else if (state == ISipDevice::DS_DISABLED)
					deviceElem.appendChild(update.createTextNode("disabled"));
				else if (state == ISipDevice::DS_UNAVAIL)
					deviceElem.appendChild(update.createTextNode("unavail"));
			}
		}

		FStanzaProcessor->sendStanzaOut(streamJid(),update);
	}
}

void SipCall::changeDeviceState(int AType, int AState)
{
	LogDetail(QString("[SipCall] Device(type=%1) state changed to %2").arg(AType).arg(AState));
	FDeviceStates[AType] = (ISipDevice::State)AState;
	emit deviceStateChanged(AType,AState);
}

void SipCall::changeDeviceProperty(int AType, int AProperty, const QVariant &AValue)
{
	FDeviceProperties[AType][AProperty] = AValue;
	emit devicePropertyChanged(AType,AProperty,AValue);
}

void SipCall::setCallState(CallState AState)
{
	if (FState != AState)
	{
		FState = AState;
		LogDetail(QString("[SipCall] Call state changed to '%1', sid='%2'").arg(AState).arg(sessionId()));
		if (AState == CS_CALLING)
		{
			if (FStanzaProcessor)
			{
				IStanzaHandle handle;
				handle.handler = this;
				handle.order = SHO_DEFAULT;
				handle.streamJid = streamJid();
				handle.direction = IStanzaHandle::DirectionIn;
				handle.conditions.append(QString(SHC_CALL_ACCEPT).arg(sessionId()));
				FSHICallAccept = FStanzaProcessor->insertStanzaHandle(handle);

				handle.conditions.clear();
				handle.conditions.append(QString(SHC_DEVICE_STATES).arg(sessionId()));
				FSHIDeviceStates = FStanzaProcessor->insertStanzaHandle(handle);
			}
			FRingTimer.start(RING_TIMEOUT);
		}
		else if (AState == CS_CONNECTING)
		{
			if (isDirectCall())
				FRingTimer.start(RING_TIMEOUT);

			FAccountId = FSipManager->sipAccountId(streamJid());
			if (FAccountId == -1)
			{
				if (!FSipManager->setSipAccountRegistration(streamJid(),true))
					continueAfterRegistration(false);
				else
					FAccountId = FSipManager->sipAccountId(streamJid());
			}
			else
			{
				continueAfterRegistration(true);
			}
		}
		else if (AState == CS_TALKING)
		{
			if (deviceState(ISipDevice::DT_LOCAL_CAMERA)==ISipDevice::DS_DISABLED && deviceProperty(ISipDevice::DT_LOCAL_CAMERA,ISipDevice::LCP_AUTO_START).toBool())
				setDeviceState(ISipDevice::DT_LOCAL_CAMERA,ISipDevice::DS_ENABLED);
			FStartCallTime = QDateTime::currentDateTime();
		}
		else if (AState == CS_FINISHED)
		{
			FStopCallTime = QDateTime::currentDateTime();
		}
		else if (AState == CS_ERROR)
		{
			FStopCallTime = QDateTime::currentDateTime();
		}
		emit stateChanged(FState);
	}
}

void SipCall::setCallError(ErrorCode ACode)
{
	if (FErrorCode == EC_EMPTY)
	{
		LogDetail(QString("[SipCall] Call error changed to '%1', sid='%2'").arg(ACode).arg(sessionId()));
		FErrorCode = ACode;
		setCallState(CS_ERROR);
	}
}

void SipCall::continueAfterRegistration(bool ARegistered)
{
	FAccountId = FSipManager->sipAccountId(streamJid());

	if (role() == CR_INITIATOR)
	{
		if (ARegistered)
		{
			if (FStanzaProcessor && !isDirectCall())
			{
				notifyActiveDestinations("accepted");
				Stanza result = FStanzaProcessor->makeReplyResult(FAcceptStanza);
				FStanzaProcessor->sendStanzaOut(streamJid(), result);
			}
			sipCallTo(FContactJid);
		}
		else
		{
			if (FStanzaProcessor && !isDirectCall())
			{
				notifyActiveDestinations("caller_error");
				ErrorHandler err(ErrorHandler::RECIPIENT_UNAVAILABLE);
				Stanza error = FStanzaProcessor->makeReplyError(FAcceptStanza,err);
				FStanzaProcessor->sendStanzaOut(streamJid(), error);
			}
			setCallError(EC_CONNECTIONERR);
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
				queryElem.setAttribute("peer", streamJid().pBare());
				if (FStanzaProcessor->sendStanzaRequest(this, streamJid(), accept, CALL_REQUEST_TIMEOUT))
					FCallRequests.insert(accept.id(), contactJid());
				else
					setCallError(EC_CONNECTIONERR);
			}
			else
			{
				Stanza error("iq");
				error.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = error.addElement("query", NS_RAMBLER_PHONE);
				queryElem.setAttribute("type", "callee_error");
				queryElem.setAttribute("sid", sessionId());
				FStanzaProcessor->sendStanzaOut(streamJid(),error);
				setCallError(EC_CONNECTIONERR);
			}
		}
	}
}

void SipCall::notifyActiveDestinations(const QString &AType)
{
	foreach(Jid destination, FActiveDestinations)
	{
		if (FStanzaProcessor && destination!=FContactJid)
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
	pj_status_t status;
	char uriTmp[512];

	pj_ansi_sprintf(uriTmp, "sip:%s", AContactJid.prepared().eBare().toAscii().constData());
	pj_str_t uri = pj_str((char*)uriTmp);

	if (FCallId == -1)
	{
		pjsua_call_setting call_setting;
		pjsua_call_setting_default(&call_setting);

		if (FContactJid.domain() == SIP_DOMAIN)
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
		if (status == PJ_SUCCESS)
		{
			FCallId = id;
			LogDetail(QString("[SipCall]: Starting SIP call to '%1'").arg(uriTmp));
		}
		else
		{
			LogError(QString("[SipCall]: Failed to start SIP call to '%1', status='%2', error='%3'").arg(uriTmp).arg(status).arg(SipManager::resolveErrorCode(status)));
			setCallError(EC_CONNECTIONERR);
		}
	}
	else
	{
		LogError(QString("[SipCall]: Failed to start SIP call to '%1': Call already started").arg(uriTmp));
	}
}

void SipCall::updateDeviceStates()
{
	if (FCallId != -1)
	{
		pjsua_call_info ci;
		if (pjsua_call_get_info(FCallId, &ci) == PJ_SUCCESS)
		{
			QMap<int, ISipDevice::State> newStates;
			for (unsigned media_index=0; media_index<ci.media_cnt; ++media_index)
			{
				if (ci.media[media_index].type == PJMEDIA_TYPE_AUDIO)
				{
					if (ci.media[media_index].dir & PJMEDIA_DIR_ENCODING)
						newStates.insert(ISipDevice::DT_LOCAL_MICROPHONE,!pjsua_aud_stream_pause_state(FCallId,PJMEDIA_DIR_ENCODING) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
					if (ci.media[media_index].dir & PJMEDIA_DIR_DECODING)
						newStates.insert(ISipDevice::DT_REMOTE_MICROPHONE,!pjsua_aud_stream_pause_state(FCallId,PJMEDIA_DIR_DECODING) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
				}
				else if (ci.media[media_index].type == PJMEDIA_TYPE_VIDEO)
				{
					if (ci.media[media_index].dir & PJMEDIA_DIR_ENCODING)
						newStates.insert(ISipDevice::DT_LOCAL_CAMERA,pjsua_call_vid_stream_is_running(FCallId,media_index,PJMEDIA_DIR_ENCODING) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
					if (ci.media[media_index].dir & PJMEDIA_DIR_DECODING)
						newStates.insert(ISipDevice::DT_REMOTE_CAMERA,pjsua_call_vid_stream_is_running(FCallId,media_index,PJMEDIA_DIR_DECODING) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
				}
			}

			bool hasUpdates = false;
			foreach(ISipDevice::Type type, QList<ISipDevice::Type>()<<ISipDevice::DT_LOCAL_CAMERA<<ISipDevice::DT_LOCAL_MICROPHONE)
			{
				ISipDevice::State newState = newStates.value(type,ISipDevice::DS_UNAVAIL);
				if (deviceState(type) != newState)
				{
					hasUpdates = true;
					changeDeviceState(type,newState);
				}
			}

			if (hasUpdates)
				sendLocalDeviceStates();
		}
	}
}

void SipCall::updateCallState(int AState)
{
	setCallState((CallState)AState);
}

void SipCall::updateDeviceProperty(int AType, int AProperty, const QVariant &AVaule)
{
	changeDeviceProperty(AType,AProperty,AVaule);
}

void SipCall::onRingTimerTimeout()
{
	if (state()==CS_CALLING || state()==CS_CONNECTING)
	{
		LogDetail(QString("[SipCall] Ring timer timed out, sid='%1'").arg(sessionId()));
		if (role() == CR_INITIATOR)
		{
			if (isDirectCall())
				setCallError(EC_NOANSWER);
		}
		else if (role() == CR_RESPONDER)
		{
			if (FStanzaProcessor && !isDirectCall())
			{
				Stanza timeout("iq");
				timeout.setTo(contactJid().eFull()).setType("set").setId(FStanzaProcessor->newId());
				QDomElement queryElem = timeout.addElement("query",NS_RAMBLER_PHONE);
				queryElem.setAttribute("type","timeout_error");
				queryElem.setAttribute("sid",sessionId());
				FStanzaProcessor->sendStanzaOut(streamJid(),timeout);
			}
			setCallError(EC_NOANSWER);
		}
	}
}

void SipCall::onDelayedRejection()
{
	setCallState(CS_CALLING);
	rejectCall(FRejectCode);
}

void SipCall::onSipAccountRegistrationChanged(int AAccountId, bool ARegistered)
{
	if (FAccountId == AAccountId)
	{
		if (state() == CS_CONNECTING)
			continueAfterRegistration(ARegistered);
	}
}
