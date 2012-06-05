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
	FContactJid = AContactJid;
	init(ASipManager, AStanzaProcessor, AXmppStream, ASessionId);
	
	FRole = CR_RESPONDER;

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
//	if (findCalls(FStreamJid).isEmpty())
//		FSipManager->unregisterAtServer(FStreamJid);
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
		return tr("Remote user could not accept call");
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

ISipDevice SipCall::activeDevice(ISipDevice::Type AType) const
{
	return FDevices.value(AType);
}

bool SipCall::setActiveDevice(ISipDevice::Type AType, const ISipDevice &ADevice)
{
	if (ADevice.type == AType)
	{
		if (activeDevice(AType) != ADevice)
		{
			LogDetail(QString("[SipCall::setActiveDevice] Active device(type=%1) changed to '%2', index=%3").arg(AType).arg(ADevice.name).arg(ADevice.index));
			FDevices.insert(AType,ADevice);
			emit activeDeviceChanged(AType);
		}
		return true;
	}
	else
	{
		LogError(QString("[SipCall::setActiveDevice] Failed to set active device: invalid device type=%1").arg(ADevice.type));
	}
	return false;
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
			if(AState == ISipDevice::DS_ENABLED)
				pjstatus = pjsua_call_set_vid_strm(FCallId, PJSUA_CALL_VID_STRM_START_TRANSMIT, NULL);
			else
				pjstatus = pjsua_call_set_vid_strm(FCallId, PJSUA_CALL_VID_STRM_STOP_TRANSMIT, NULL);
		}
		else if (AType == ISipDevice::DT_LOCAL_MICROPHONE)
		{
			pjstatus = pjsua_aud_stream_pause_state(FCallId, PJMEDIA_DIR_CAPTURE, (AState == ISipDevice::DS_ENABLED) ? false : true);
		}
		else if (AType == ISipDevice::DT_REMOTE_CAMERA)
		{
			pjsua_call_setting call_setting;
			pjsua_call_setting_default(&call_setting);
			call_setting.vid_cnt = (AState == ISipDevice::DS_ENABLED) ? 1 : 0;
			pjstatus = pjsua_call_reinvite2(FCallId, &call_setting, NULL);
		}
		else if (AType == ISipDevice::DT_REMOTE_MICROPHONE)
		{
			pjsua_call_info ci;
			pjsua_call_get_info(FCallId, &ci);
			pjstatus = pjsua_conf_adjust_rx_level(ci.conf_slot, AState==ISipDevice::DS_ENABLED ? deviceProperty(AType, ISipDevice::RMP_VOLUME).toFloat() : 0.0f);
		}
		if (pjstatus == PJ_SUCCESS)
		{
			LogDetail(QString("[SipCall::setDeviceState] Device(type=%1) state changed to %2").arg(AType).arg(AState));
			changeDeviceState(AType,AState);
			return true;
		}
		else
		{
			LogError(QString("[SipCall::setDeviceState]: Failed to change device(type=%1) state to %2, pjstatus=%3, pjerror='%4'").arg(AType).arg(AState).arg(pjstatus).arg(SipManager::resolveErrorCode(pjstatus)));
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

bool SipCall::acceptIncomingCall(int ACallId)
{
	if (role() == CR_RESPONDER)
	{
		if (state() == CS_CONNECTING)
		{
			FCallId = ACallId;
			pjsua_call_answer(FCallId, PJSIP_SC_OK, NULL, NULL);
			initDevices();
			return true;
		}
	}
	return false;
}

void SipCall::onCallState(int call_id, void *e)
{
	Q_UNUSED(e)
	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);

	if(ci.state == PJSIP_INV_STATE_CONFIRMED)
	{
		setCallState(CS_TALKING);
	}
	else if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
	{
		setCallState(CS_FINISHED);
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

int SipCall::onMyPutFrameCallback(void *frame, int w, int h, int stride)
{
	// TODO: check implementation
	pjmedia_frame * _frame = (pjmedia_frame *)frame;
	if(_frame->type == PJMEDIA_FRAME_TYPE_VIDEO)
	{
		int dstSize = w * h * 3;
		unsigned char * dst = new unsigned char[dstSize];

		FC::YUV420PtoRGB32(w, h, stride, (unsigned char *)_frame->buf, dst, dstSize);
		QImage remoteImage = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();
		delete [] dst;

		changeDeviceProperty(ISipDevice::DT_REMOTE_CAMERA,ISipDevice::RCP_CURRENTFRAME,remoteImage);
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

		changeDeviceProperty(ISipDevice::DT_LOCAL_CAMERA, ISipDevice::LCP_CURRENTFRAME,previewImage);
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
	FState = CS_INIT;
	FErrorCode = EC_EMPTY;
	FRejectCode = RC_EMPTY;

	FRingTimer.setSingleShot(true);
	connect(&FRingTimer, SIGNAL(timeout()), SLOT(onRingTimerTimeout()));

	connect(FSipManager->instance(), SIGNAL(registeredAtServer(const QString &)), SLOT(onRegisteredAtServer(const QString &)));
	connect(FSipManager->instance(), SIGNAL(unregisteredAtServer(const QString &)), SLOT(onUnRegisteredAtServer(const QString &)));
	connect(FSipManager->instance(), SIGNAL(registrationAtServerFailed(const QString &)), SLOT(onRegistraitionAtServerFailed(const QString &)));

	FCallInstances.append(this);
}

void SipCall::initDevices()
{
	FSipManager->updateAvailDevices();

	if (FSipManager->isDevicePresent(ISipDevice::DT_LOCAL_CAMERA))
	{
		if (setActiveDevice(ISipDevice::DT_LOCAL_CAMERA,FSipManager->activeDevice(ISipDevice::DT_LOCAL_CAMERA)))
			changeDeviceState(ISipDevice::DT_LOCAL_CAMERA,ISipDevice::DS_ENABLED);
	}
	else
	{
		LogError("[SipCall::initDevices]: No local camera found!");
	}

	if (FSipManager->isDevicePresent(ISipDevice::DT_REMOTE_CAMERA))
	{
		if (setActiveDevice(ISipDevice::DT_REMOTE_CAMERA,FSipManager->activeDevice(ISipDevice::DT_REMOTE_CAMERA)))
			changeDeviceState(ISipDevice::DT_REMOTE_CAMERA,ISipDevice::DS_ENABLED);
	}
	else
	{
		LogError("[SipCall::initDevices]: No remote camera found!");
	}


	if (FSipManager->isDevicePresent(ISipDevice::DT_LOCAL_MICROPHONE))
	{
		if (setActiveDevice(ISipDevice::DT_LOCAL_MICROPHONE,FSipManager->activeDevice(ISipDevice::DT_LOCAL_MICROPHONE)))
		{
			changeDeviceState(ISipDevice::DT_LOCAL_MICROPHONE,ISipDevice::DS_ENABLED);
			changeDeviceProperty(ISipDevice::DT_LOCAL_MICROPHONE,ISipDevice::LMP_MAX_VOLUME,MAX_VOLUME);
			changeDeviceProperty(ISipDevice::DT_LOCAL_MICROPHONE,ISipDevice::LMP_VOLUME,DEF_VOLUME);
		}
	}
	else
	{
		LogError("[SipCall::initDevices]: No local microphone found!");
	}

	if (FSipManager->isDevicePresent(ISipDevice::DT_REMOTE_MICROPHONE))
	{
		if (setActiveDevice(ISipDevice::DT_REMOTE_MICROPHONE,FSipManager->activeDevice(ISipDevice::DT_REMOTE_MICROPHONE)))
		{
			changeDeviceState(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::DS_ENABLED);
			changeDeviceProperty(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::RMP_MAX_VOLUME,MAX_VOLUME);
			changeDeviceProperty(ISipDevice::DT_REMOTE_MICROPHONE,ISipDevice::RMP_VOLUME,DEF_VOLUME);
		}
	}
	else
	{
		LogError("[SipCall::initDevices]: No remote microphone found!");
	}
}

void SipCall::changeDeviceState(int AType, int AState)
{
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
			}
			FRingTimer.start(RING_TIMEOUT);
		}
		else if (AState == CS_CONNECTING)
		{
			if (isDirectCall())
				FRingTimer.start(RING_TIMEOUT);

			if (!FSipManager->isRegisteredAtServer(streamJid()))
			{
				if (!FSipManager->registerAtServer(streamJid()))
					continueAfterRegistration(false);
			}
			else
			{
				continueAfterRegistration(true);
			}
		}
		else if (AState == CS_TALKING)
		{
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
	FAccountId = FSipManager->registeredAccountId(streamJid());

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
			initDevices();
			LogDetail(QString("[SipCall::sipCallTo]: SIP call to '%1'").arg(uriTmp));
		}
		else
		{
			if (status == PJMEDIA_EAUD_NODEFDEV)
				LogError(QString("[SipCall::sipCallTo]: Default device not found!"));
			else
				LogError(QString("[SipCall::sipCallTo]: pjsua_call_make_call() returned status (%1) %2, uri is \'%3\'").arg(status).arg(SipManager::resolveErrorCode(status)).arg(uriTmp));
			setCallError(EC_CONNECTIONERR);
		}
	}
	else
	{
		LogError(QString("[SipCall::sipCallTo]: Call is already active with stream jid %1 and contact %2").arg(streamJid().full(), contactJid().full()));
	}
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

void SipCall::onRegisteredAtServer(const QString &AAccount)
{
	if (AAccount == streamJid().pBare())
	{
		if (state() == CS_CONNECTING)
			continueAfterRegistration(true);
	}
}

void SipCall::onUnRegisteredAtServer(const QString &AAccount)
{
	Q_UNUSED(AAccount)
	// TODO: implementation
}

void SipCall::onRegistraitionAtServerFailed(const QString &AAccount)
{
	if (AAccount == streamJid().pBare())
	{
		if (state() == CS_CONNECTING)
			continueAfterRegistration(false);
	}
}
