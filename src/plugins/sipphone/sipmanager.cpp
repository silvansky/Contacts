#include "sipmanager.h"

#include <QLabel>
#include <QProcess>
#include <QTextCodec>

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/sipcallhandlerorders.h>
#include <definitions/gateserviceidentifiers.h>
#include <utils/log.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include <utils/custombordercontainer.h>

#include "sipcall.h"
#include "frameconverter.h"
#include "pjsipdefines.h"
#include "pjsipcallbacks.h"

#include "testcallwidget.h"
#include "videocallwindow.h"

#if defined(Q_WS_WIN)
# include <windows.h>
#elif defined(Q_WS_MAC)
# include <utils/macutils.h>
#endif

#if defined(DEBUG_ENABLED)
# include <QDebug>
#endif

#define ADR_STREAM_JID           Action::DR_StreamJid
#define ADR_WINDOW_METAID        Action::DR_Parametr1
#define ADR_DESTINATIONS         Action::DR_Parametr2
#define ADR_PHONE_NUMBER         Action::DR_Parametr3

/* The PJSIP module instance. */
static pjsip_module mod_default_handler =
{
	NULL, NULL,				/* prev, next.		*/
	{ (char*)"mod-default-handler", 19 },	/* Name.		*/
	-1,					/* Id			*/
	PJSIP_MOD_PRIORITY_APPLICATION,		/* Priority	        */
	NULL,					/* load()		*/
	NULL,					/* start()		*/
	NULL,					/* stop()		*/
	NULL,					/* unload()		*/
	NULL,					/* on_rx_request()	*/
	NULL,					/* on_rx_response()	*/
	NULL,					/* on_tx_request.	*/
	NULL,					/* on_tx_response()	*/
	NULL,					/* on_tsx_state()	*/
};

/**************
 * SipManager *
 **************/

#define SHC_SIP_QUERY "/iq[@type='set']/query[@type='request'][@xmlns='" NS_RAMBLER_PHONE "']"

SipManager * SipManager::inst = NULL; // callback instance

SipManager::SipManager() :
	QObject(NULL)
{
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FMetaContacts = NULL;
	FPluginManager = NULL;
	FStanzaProcessor = NULL;
	FRosterChanger = NULL;
	FGateways = NULL;

	inst = this;
	FSHISipQuery = -1;
	FSipStackCreated = false;

#if defined(HAS_VIDEO_SUPPORT) && (HAS_VIDEO_SUPPORT != 0)
	PJCallbacks::registerFrameCallbacks(myframe);
#endif
}

SipManager::~SipManager()
{
	destroySipStack();
}

QObject *SipManager::instance()
{
	return this;
}

QUuid SipManager::pluginUuid() const
{
	return SIPMANAGER_UUID;
}

void SipManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SIP Manager");
	APluginInfo->description = tr("Allows to make voice and video calls over SIP protocol");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Gorshkov V.A.";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SipManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		if(FMetaContacts)
		{
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowCreated(IMetaTabWindow *)), SLOT(onMetaTabWindowCreated(IMetaTabWindow *)));
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowDestroyed(IMetaTabWindow *)), SLOT(onMetaTabWindowDestroyed(IMetaTabWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams*>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onXmppStreamAboutToClose(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	return FStanzaProcessor!=NULL;
}

bool SipManager::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipPhone;
		sipPhone.active = true;
		sipPhone.var = NS_RAMBLER_PHONE;
		sipPhone.name = tr("SIP Phone 2");
		sipPhone.description = tr("SIP voice and video calls");
		FDiscovery->insertDiscoFeature(sipPhone);
	}
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_SIP_QUERY);
		FSHISipQuery = FStanzaProcessor->insertStanzaHandle(shandle);
	}

	insertSipCallHandler(SCHO_SIPMANAGER_VIDEOCALLS, this);

	return true;
}

bool SipManager::initSettings()
{
	return true;
}

bool SipManager::startPlugin()
{
	return true;
}

bool SipManager::isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery && FDiscovery->discoInfo(AStreamJid, AContactJid).features.contains(NS_RAMBLER_PHONE);
}

ISipCall *SipManager::newCall(const Jid &AStreamJid, const QString &APhoneNumber)
{
	SipCall *call = new SipCall(this, AStreamJid, APhoneNumber, QUuid::createUuid().toString());
	emit sipCallCreated(call);
	return call;
}

ISipCall *SipManager::newCall(const Jid &AStreamJid, const QList<Jid> &ADestinations)
{
	SipCall *call = new SipCall(this, FStanzaProcessor, AStreamJid, ADestinations, QUuid::createUuid().toString());
	emit sipCallCreated(call);
	return call;
}

QList<ISipCall*> SipManager::findCalls(const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId) const
{
	return SipCall::findCalls(AStreamJid, AContactJid, ASessionId);
}

int SipManager::registeredAccountId(const Jid &AStreamJid) const
{
	return FAccounts.value(AStreamJid.bare(), -1);
}

bool SipManager::isRegisteredAtServer(const Jid &AStreamJid) const
{
	return FAccounts.value(AStreamJid.bare(), -1) != -1;
}

bool SipManager::registerAtServer(const Jid &AStreamJid)
{
	IXmppStream *stream = FXmppStreams ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (stream && !isRegisteredAtServer(AStreamJid) && createSipStack())
	{
		// Create account
		pjsua_acc_config acc_cfg;
		pjsua_acc_config_default(&acc_cfg);

		char idtmp[1024];
		QString idString = stream->streamJid().pBare();
		pj_ansi_snprintf(idtmp, sizeof(idtmp), "sip:%s", idString.toAscii().constData());

		acc_cfg.id = pj_str((char*)idtmp);

		char reg_uritmp[1024];
		pj_ansi_snprintf(reg_uritmp, sizeof(reg_uritmp), "sip:%s", stream->streamJid().pDomain().toAscii().constData());
		acc_cfg.reg_uri = pj_str((char*)reg_uritmp);

		acc_cfg.cred_count = 1;
		acc_cfg.cred_info[0].realm = pj_str((char*)"*");
		acc_cfg.cred_info[0].scheme = pj_str((char*)"digest");

		char usernametmp[512];
		pj_ansi_snprintf(usernametmp, sizeof(usernametmp), "%s", idString.toAscii().constData());
		acc_cfg.cred_info[0].username = pj_str((char*)usernametmp);

		char passwordtmp[512];
		pj_ansi_strncpy(passwordtmp, stream->password().toAscii().constData(), sizeof(passwordtmp));
		acc_cfg.cred_info[0].data = pj_str((char*)passwordtmp);

		acc_cfg.vid_cap_dev = DEFAULT_CAP_DEV;
		acc_cfg.vid_rend_dev = DEFAULT_REND_DEV;
		acc_cfg.vid_in_auto_show = PJ_TRUE;
		acc_cfg.vid_out_auto_transmit = PJ_TRUE;
		acc_cfg.register_on_acc_add = PJ_FALSE;

		pjsua_acc_id acc;
		pj_status_t status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc);
		if (status == PJ_SUCCESS)
		{
			status = pjsua_start();
			if (status == PJ_SUCCESS)
			{
				if (pjsua_get_pjsip_endpt())
				{
					PJCallbacks::registerModuleCallbacks(mod_default_handler);
					status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &mod_default_handler);
					if (status == PJ_SUCCESS)
					{
						status = pjsua_acc_set_registration(acc, true);
						if (status == PJ_SUCCESS)
						{
							LogDetail("[SipManager]: Starting registration on SIP server");
							return true;
						}
						else
						{
							LogError(QString("[SipManager]: Failed to start registration pjsua_acc_set_registration returned %1.").arg(status));
						}
					}
					else
					{
						LogError(QString("[SipManager]: Failed to register pjsip module! pjsip_endpt_register_module() returned %1.").arg(status));
					}
				} // pjsua_get_pjsip_endpt
				else
				{
					LogError("[SipManager]: pjsua_get_pjsip_endpt() invalid!");
				}
			} // pjsua_start
			else
			{
				LogError(QString("[SipManager]: pjsua_start() failed with status %1!").arg(status));
			}
		} // pjsua_acc_add
		else
		{
			LogError(QString("[SipManager]: pjsua_acc_add() failed with status %1!").arg(status));
		}
	}
	return false;
}

bool SipManager::unregisterAtServer(const Jid &AStreamJid)
{
	// TODO: check implementation
	int acc = FAccounts.value(AStreamJid.bare(), -1);
	if (acc != -1)
	{
		pj_status_t status = pjsua_acc_set_registration(acc, false);
		if (status == PJ_SUCCESS)
		{
			FAccounts.remove(AStreamJid.bare());
			emit unregisteredAtServer(AStreamJid.bare());
			return true;
		}
		else
		{
			LogError(QString("[SipManager]: Failed to unregister account '%1' on SIP server, pjsua_acc_set_registration() returned %2").arg(AStreamJid.bare()).arg(status));
		}
	}
	return false;
}

void SipManager::showSystemSoundPreferences() const
{
#if defined(Q_WS_WIN)
	OSVERSIONINFO m_osinfo;
	ZeroMemory(&m_osinfo, sizeof(m_osinfo));
	m_osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx((LPOSVERSIONINFO) &m_osinfo))
	{
		if(m_osinfo.dwMajorVersion < 6)
		{
			QProcess::startDetached("sndvol32.exe");
		}
		else
		{
			QProcess::startDetached("sndvol.exe");
		}
	}
#elif defined (Q_WS_MAC)
	QProcess::startDetached("open -W \"/System/Library/PreferencePanes/Sound.prefPane\"");
#endif
}

QList<ISipDevice> SipManager::availDevices(ISipDevice::Type AType, bool ARefresh) const
{
	static QList<ISipDevice> audioInputDevices;
	static QList<ISipDevice> audioOutputDevices;
	static QList<ISipDevice> videoInputDevices;
	static QList<ISipDevice> videoOutputDevices;
	if (FSipStackCreated)
	{
		if (ARefresh)
		{
			pj_status_t status = pjmedia_aud_dev_refresh();
			if (status != PJ_SUCCESS)
			{
				LogError(QString("[SipManager::availDevices]: Failed to refresh audio devices! pjmedia_aud_dev_refresh() returned %1").arg(status));
			}
			status = pjmedia_vid_dev_refresh();
			if (status != PJ_SUCCESS)
			{
				LogError(QString("[SipManager::availDevices]: Failed to refresh video devices! pjmedia_vid_dev_refresh() returned %1").arg(status));
			}
		}

		if (ARefresh || audioInputDevices.isEmpty() || audioOutputDevices.isEmpty())
		{
			audioInputDevices.clear();
			audioOutputDevices.clear();
			uint numDevices = pjmedia_aud_dev_count();
			for (uint i = 0; i < numDevices; i++)
			{
				pjmedia_aud_dev_info info;
				pj_status_t pjstatus = pjmedia_aud_dev_get_info(i, &info);
				if (pjstatus == PJ_SUCCESS)
				{
					if (info.input_count)
					{
						ISipDevice d;
						d.type = ISipDevice::DT_LOCAL_MICROPHONE;
						d.id = i;
						d.name = QString::fromUtf8(info.name);
						audioInputDevices << d;
						LogDetail(QString("[SipManager::availDevices]: Found INPUT audio device with id %1 and name %2. Driver is %3").arg(i).arg(d.name).arg(info.driver));
					}
					if (info.output_count)
					{
						ISipDevice d;
						d.type = ISipDevice::DT_REMOTE_MICROPHONE;
						d.id = i;
						d.name = QString::fromUtf8(info.name);
						audioOutputDevices << d;
						LogDetail(QString("[SipManager::availDevices]: Found OUTPUT audio device with id %1 and name %2. Driver is %3").arg(i).arg(d.name).arg(info.driver));
					}
				}
				else
				{
					LogError(QString("[SipManager::availDevices]: Failed to get device info for device %1! pjmedia_aud_dev_get_info() returned %2").arg(i).arg(pjstatus));
				}
			}
		}
		if (ARefresh || videoInputDevices.isEmpty() || videoOutputDevices.isEmpty())
		{
			videoInputDevices.clear();
			videoOutputDevices.clear();
			uint numDevices = pjmedia_vid_dev_count();
			for (uint i = 0; i < numDevices; i++)
			{
				pjmedia_vid_dev_info info;
				pj_status_t pjstatus = pjmedia_vid_dev_get_info(i, &info);
				if (pjstatus == PJ_SUCCESS)
				{
#if defined(Q_WS_MAC)
					QString name = convertFromMacCyrillic(info.name);
#else
					QString name(info.name);
#endif
					if (info.dir == PJMEDIA_DIR_CAPTURE)
					{
						ISipDevice d;
						d.type = ISipDevice::DT_LOCAL_CAMERA;
						d.id = i;
						d.name = name;
						videoInputDevices << d;
						LogDetail(QString("[SipManager::availDevices]: Found INPUT video device with id %1 and name %2. Driver is %3").arg(i).arg(name).arg(info.driver));
					}
					else if (info.dir == PJMEDIA_DIR_PLAYBACK)
					{
						ISipDevice d;
						d.type = ISipDevice::DT_REMOTE_CAMERA;
						d.id = i;
						d.name = name;
						videoOutputDevices << d;
						LogDetail(QString("[SipManager::availDevices]: Found OUTPUT video device with id %1 and name %2. Driver is %3").arg(i).arg(name).arg(info.driver));
					}
					else if (info.dir == PJMEDIA_DIR_ENCODING_DECODING)
					{
						ISipDevice d;
						d.type = ISipDevice::DT_LOCAL_CAMERA;
						d.id = i;
						d.name = name;
						videoInputDevices << d;
						d.type = ISipDevice::DT_LOCAL_CAMERA;
						videoOutputDevices << d;
						LogDetail(QString("[SipManager::availDevices]: Found INPUT/OUTPUT video device with id %1 and name %2. Driver is %3").arg(i).arg(name).arg(info.driver));
					}
				}
				else
				{
					LogError(QString("[SipManager::availDevices]: Failed to get device info for device %1! pjmedia_vid_dev_get_info() returned %2").arg(i).arg(pjstatus));
				}
			}
		}
		switch (AType)
		{
		case ISipDevice::DT_LOCAL_MICROPHONE:
			return audioInputDevices;
			break;
		case ISipDevice::DT_REMOTE_MICROPHONE:
			return audioOutputDevices;
			break;
		case ISipDevice::DT_LOCAL_CAMERA:
			return videoInputDevices;
			break;
		case ISipDevice::DT_REMOTE_CAMERA:
			return videoOutputDevices;
			break;
		default:
			break;
		}
	}
	return QList<ISipDevice>();
}

ISipDevice SipManager::getDevice(ISipDevice::Type AType, int ADeviceId) const
{
	Q_UNUSED(AType)
	Q_UNUSED(ADeviceId)
	// TODO: implementation
	return ISipDevice();
}

void SipManager::insertSipCallHandler(int AOrder, ISipCallHandler *AHandler)
{
	FCallHandlers.insertMulti(AOrder, AHandler);
}

void SipManager::removeSipCallHandler(int AOrder, ISipCallHandler *AHandler)
{
	FCallHandlers.remove(AOrder,AHandler);
}

bool SipManager::handleSipCall(int AOrder, ISipCall *ACall)
{
	if (AOrder == SCHO_SIPMANAGER_VIDEOCALLS)
	{
		VideoCallWindow *window = new VideoCallWindow(FPluginManager,ACall);
		WidgetManager::showActivateRaiseWindow(window->window());
		WidgetManager::alignWindow(window->window(),Qt::AlignCenter);
		ACall->startCall();
		return true;
	}
	return false;
}

bool SipManager::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISipQuery == AHandleId)
	{
		AAccept = true;
		QDomElement queryElem = AStanza.firstElement("query",NS_RAMBLER_PHONE);
		QString type = queryElem.attribute("type");
		if (type == "request")
		{
			QString sessionId = queryElem.attribute("sid");
			if (sessionId.isEmpty())
			{
				LogError(QString("[SipManager]: Invalid incoming call request from %1 to %2, sid='%3'").arg(AStanza.from(), AStreamJid.full(), sessionId));
				ErrorHandler err(ErrorHandler::BAD_REQUEST);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else if (!findCalls(Jid::null,Jid::null,sessionId).isEmpty())
			{
				LogError(QString("[SipManager]: Duplicated sessionID in incoming call request from %1 to %2, sid='%3'").arg(AStanza.from(), AStreamJid.full(), sessionId));
				ErrorHandler err(ErrorHandler::CONFLICT);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else if (!findCalls(Jid::null,AStanza.from()).isEmpty())
			{
				LogError(QString("[SipManager]: Duplicated incoming call request from %1 to %2, sid='%3'").arg(AStanza.from(), AStreamJid.full(), sessionId));
				ErrorHandler err(ErrorHandler::FORBIDDEN);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else if (!handleIncomingCall(AStreamJid, AStanza.from(), sessionId))
			{
				LogError(QString("[SipManager]: Failed to handle incoming call request from %1 to %2, sid='%3'").arg(AStanza.from(), AStreamJid.full(), sessionId));
				ErrorHandler err(ErrorHandler::UNEXPECTED_REQUEST);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else
			{
				Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
				FStanzaProcessor->sendStanzaOut(AStreamJid,result);
			}
		}
	}
	return false;
}

SipManager *SipManager::callbackInstance()
{
	return inst;
}

void SipManager::onRegState(int acc_id)
{
	// TODO: check implementation
	if (acc_id == -1)
	{
		LogDetail("[SipManager::onRegState]: Invalid account Id!");
		return;
	}

	pjsua_acc_info info;
	pjsua_acc_get_info(acc_id, &info);

	QString account = QString("%1").arg(info.acc_uri.ptr);
	account.remove(0, 4); // remove "sip:"
	if (info.status == PJSIP_SC_OK)
	{
		LogDetail(QString("[SipManager]: Successfully registered on SIP server account \'%1\', online: %2, has registration: %3").arg(account).arg(info.online_status).arg(info.has_registration));
		FAccounts.insert(account, acc_id);
		//availDevices(ISipDevice::DT_LOCAL_MICROPHONE);
		emit registeredAtServer(account);
	}
	else
	{
		LogError(QString("[SipManager]: Failed to register on SIP server account '%1'").arg(account));
		emit registrationAtServerFailed(account);
	}
}

void SipManager::onRegState2(int acc_id, void *info)
{
	// TODO: check this MAGIC implementation
	LogDetail(QString("[SipManager]: Registration status %1, account: %2").arg(((pjsua_reg_info*)info)->cbparam->status).arg(acc_id));
	int i;
	i = ((pjsua_reg_info*)info)->cbparam->code;
	i++;
}

void SipManager::onIncomingCall(int acc_id, int call_id, void *rdata)
{
	Q_UNUSED(acc_id);
	Q_UNUSED(rdata);
	// TODO: check implementation

	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);
	QString callerId = QString("%1").arg(ci.remote_info.ptr);
	QString receiverId = QString("%1").arg(ci.local_info.ptr);

	callerId.remove(0,5);callerId.chop(1); // remove "<sip:....>"
	receiverId.remove(0,5);receiverId.chop(1); // remove "<sip:....>"
	QList<ISipCall *> calls = findCalls(receiverId,callerId);
	SipCall *call = !calls.isEmpty() ? qobject_cast<SipCall *>(calls.value(0)->instance()) : NULL;
	if (call && call->acceptIncomingCall(call_id))
	{
		LogDetail(QString("[SipManager::onIncomingCall]: Incoming SIP call to '%1' from '%2' was accepted").arg(receiverId).arg(callerId));
	}
	else
	{
		// TODO: decline call here while we do not support direct incoming calls
		LogError(QString("[SipManager] Unexpected incoming SIP call to='%1' from='%2'").arg(receiverId,callerId));
		pj_status_t status = pjsua_call_hangup(call_id, PJSIP_SC_DECLINE, NULL, NULL);
		if (status != PJ_SUCCESS)
			LogError(QString("[SipManager::onIncomingCall]: Failed to end call! pjsua_call_hangup() returned %1").arg(status));
	}
}

bool SipManager::createSipStack()
{
	if (!FSipStackCreated)
	{
		// TODO: check implementation
		pj_status_t status;
		status = pjsua_create();
		if (status == PJ_SUCCESS)
		{
			pjsua_config ua_cfg;
			pjsua_config_default(&ua_cfg);

			pj_bzero(&ua_cfg.cb, sizeof(ua_cfg.cb));
			PJCallbacks::registerCallbacks(ua_cfg.cb);
			ua_cfg.outbound_proxy_cnt = 1;

			char proxyTmp[512];
			pj_ansi_snprintf(proxyTmp, sizeof(proxyTmp), "sip:%s", SIP_DOMAIN);
			ua_cfg.outbound_proxy[0] = pj_str((char*)proxyTmp);

			pjsua_logging_config log_cfg;
			pjsua_logging_config_default(&log_cfg);
			log_cfg.log_filename = pj_str((char*)"pjsip.log");

			pjsua_media_config med_cfg;
			pjsua_media_config_default(&med_cfg);
			med_cfg.thread_cnt = 1;

			status = pjsua_init(&ua_cfg, &log_cfg, &med_cfg);
			if (status == PJ_SUCCESS)
			{
				pjsua_transport_config udp_cfg;
				pjsua_transport_id udp_id;
				pjsua_transport_config_default(&udp_cfg);
				udp_cfg.port = SIP_PORT;

				status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &udp_cfg, &udp_id);
				if (status == PJ_SUCCESS)
				{
					pjsua_transport_info udp_info;
					status = pjsua_transport_get_info(udp_id, &udp_info);
					if (status == PJ_SUCCESS)
					{
						FSipStackCreated = true;
						LogDetail(QString("[SipManager] PJSIP stack created"));
						return true;
					} // pjsua_transport_get_info
					else
					{
						LogError(QString("[SipManager]: pjsua_transport_get_info() failed with status %1!").arg(status));
					}
				} // pjsua_transport_create
				else
				{
					LogError(QString("[SipManager]: pjsua_transport_create() failed with status %1!").arg(status));
				}
			} // pjsua_init
			else
			{
				LogError(QString("[SipManager]: pjsua_init() failed with status %1!").arg(status));
			}
			pjsua_destroy();
		}
		else
		{
			LogError(QString("[SipManager]: Failed to create pjsua! pjsua_crete() returned %1").arg(status));
		}
	}
	return false;
}

void SipManager::destroySipStack()
{
	if (FSipStackCreated)
	{
		pjsua_destroy();
		FSipStackCreated = false;
		LogDetail(QString("[SipManager] PJSIP stack destroyed"));
	}
}

bool SipManager::handleIncomingCall(const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId)
{
	LogDetail(QString("[SipManager]: Processing incoming call from %1 to %2, sid='%3'").arg(AContactJid.full(), AStreamJid.full(), ASessionId));

	// TODO: check availability of answering the call (busy) <- handler should do this
	SipCall *call = new SipCall(this,FStanzaProcessor,AStreamJid,AContactJid,ASessionId);
	emit sipCallCreated(call);

	bool handled = false;
	for (QMap<int,ISipCallHandler *>::const_iterator it=FCallHandlers.constBegin(); !handled && it!=FCallHandlers.constEnd(); it++)
		handled = it.value()->handleSipCall(it.key(),call);

	if (!handled)
	{
		call->rejectCall(ISipCall::RC_NOHANDLER);
		call->deleteLater();
	}
	
	return handled;
}

void SipManager::onXmppStreamOpened(IXmppStream *stream)
{
	TestCallWidget * w = new TestCallWidget(this, stream->streamJid());
	testCallWidgets.insert(stream->streamJid(), w);
	w->show();
}

void SipManager::onXmppStreamAboutToClose(IXmppStream *stream)
{
	TestCallWidget * w = testCallWidgets.value(stream->streamJid(), NULL);
	if (w)
	{
		w->hide();
		testCallWidgets.remove(stream->streamJid());
		w->deleteLater();
	}
}

void SipManager::onXmppStreamClosed(IXmppStream *stream)
{
	Q_UNUSED(stream)
}

void SipManager::onCallStateChanged(int AState)
{
#ifdef DEBUG_ENABLED
	qDebug() << "Call state changed to " << AState;
#endif
	ISipCall * call = (ISipCall *)sender();
	switch (AState)
	{
	case ISipCall::CS_FINISHED:
		handledCalls.removeAll(call);
		break;
	case ISipCall::CS_TALKING:
		break;
	case ISipCall::CS_ERROR:
	{
		TestCallWidget * w = testCallWidgets.values().at(0);
		w->setStatusText(call->errorString());
#ifdef DEBUG_ENABLED
		qDebug() << "Call error:" + call->errorString();
#endif
		break;
	}
	default:
		break;
	}
}

void SipManager::onCallActiveDeviceChanged(int ADeviceType)
{
	Q_UNUSED(ADeviceType)
}

void SipManager::onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState)
{
	Q_UNUSED(AState)
	// TODO: show "no camera" in case of DS_UNAVAIL/DS_DISABLED state
	if (AType == ISipDevice::DT_LOCAL_CAMERA)
	{

	}
	else if (AType == ISipDevice::DT_REMOTE_CAMERA)
	{

	}
}

void SipManager::onCallDevicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	if (AType == ISipDevice::DT_LOCAL_CAMERA)
	{
		if (AProperty == ISipDevice::LCP_CURRENTFRAME)
		{
			// TODO: set preview image to test widget
			TestCallWidget * w = testCallWidgets.values().at(0);
			w->setPreview(AValue.value<QPixmap>());
		}
	}
	else if (AType == ISipDevice::DT_REMOTE_CAMERA)
	{
		if (AProperty == ISipDevice::RCP_CURRENTFRAME)
		{
			// TODO: set remote image to test widget
			TestCallWidget * w = testCallWidgets.values().at(0);
			w->setRemoteImage(AValue.value<QPixmap>());
		}
	}
}

void SipManager::onStartVideoCall()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QList<Jid> destinations;
		foreach(QString destination ,action->data(ADR_DESTINATIONS).toStringList())
			destinations.append(destination);
		Jid streamJid = action->data(ADR_STREAM_JID).toString();

		ISipCall *call = newCall(streamJid,destinations);
		if (call)
		{
			VideoCallWindow *window = new VideoCallWindow(FPluginManager,call);
			window->sipCall()->startCall();
			WidgetManager::showActivateRaiseWindow(window->window());
			WidgetManager::alignWindow(window->window(),Qt::AlignCenter);
		}
	}
}

void SipManager::onStartPhoneCall()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QString number = action->data(ADR_PHONE_NUMBER).toString();

		ISipCall *call = newCall(streamJid,number);
		if (call)
		{
			VideoCallWindow *window = new VideoCallWindow(FPluginManager,call);
			window->sipCall()->startCall();
			WidgetManager::showActivateRaiseWindow(window->window());
			WidgetManager::alignWindow(window->window(),Qt::AlignCenter);
		}
	}
}

void SipManager::onShowAddContactDialog()
{
	Action *action = qobject_cast<Action*>(sender());
	if (FRosterChanger && action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QString metaId = action->data(ADR_WINDOW_METAID).toString();

		QWidget *widget = FRosterChanger->showAddContactDialog(streamJid);
		if (widget)
		{
			IAddContactDialog *dialog = NULL;
			if (!(dialog = qobject_cast<IAddContactDialog*>(widget)))
			{
				if (CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(widget))
					dialog = qobject_cast<IAddContactDialog*>(border->widget());
			}
			if (dialog)
			{
				IMetaRoster* mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(streamJid) : NULL;
				if (mroster)
				{
					IMetaContact contact = mroster->metaContact(metaId);
					dialog->setGroup(contact.groups.toList().value(0));
					dialog->setParentMetaContactId(metaId);
				}
			}
		}
	}
}

void SipManager::onCallMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	IMetaTabWindow *window = FCallMenus.key(menu);
	if (menu && window)
	{
		menu->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 1);

		QStringList phoneNumbers;
		QStringList destinations;
		IMetaContact contact = window->metaRoster()->metaContact(window->metaId());
		foreach(Jid itemJid, contact.items)
		{
			foreach(IPresenceItem pitem, window->metaRoster()->itemPresences(itemJid)) 
			{
				if (isCallSupported(window->metaRoster()->streamJid(),pitem.itemJid))
					destinations.append(pitem.itemJid.full());
			}

			IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemJid);
			if (descriptor.gateId == GSID_SMS)
			{
				QString number = itemJid.node();
				if (FGateways)
					number = FGateways->normalizedContactLogin(FGateways->gateDescriptorById(GSID_SMS),number);
				phoneNumbers.append(number);
			}
		}
		if (!destinations.isEmpty())
		{
			Action *videoCallAction = new Action(menu);
			videoCallAction->setText(FMetaContacts->metaContactName(contact));
			videoCallAction->setData(ADR_STREAM_JID,window->metaRoster()->streamJid().full());
			videoCallAction->setData(ADR_DESTINATIONS,destinations);
			connect(videoCallAction,SIGNAL(triggered()),SLOT(onStartVideoCall()));
			menu->addAction(videoCallAction,AG_SPCM_SIPPHONE_VIDEO_LIST);
		}

		foreach(QString number, phoneNumbers)
		{
			Action *phoneCallAction = new Action(menu);
			phoneCallAction->setText(FGateways!=NULL ? FGateways->formattedContactLogin(FGateways->gateDescriptorById(GSID_SMS),number) : number);
			phoneCallAction->setData(ADR_STREAM_JID,window->metaRoster()->streamJid().full());
			phoneCallAction->setData(ADR_PHONE_NUMBER,number);
			connect(phoneCallAction,SIGNAL(triggered()),SLOT(onStartPhoneCall()));
			menu->addAction(phoneCallAction,AG_SPCM_SIPPHONE_PHONE_LIST);
		}

		if (FRosterChanger)
		{
			Action *addContactAction = new Action(menu);
			addContactAction->setText(tr("Add Number..."));
			addContactAction->setData(ADR_STREAM_JID, window->metaRoster()->streamJid().full());
			addContactAction->setData(ADR_WINDOW_METAID, window->metaId());
			connect(addContactAction, SIGNAL(triggered(bool)), SLOT(onShowAddContactDialog()));
			menu->addAction(addContactAction, AG_SPCM_SIPPHONE_ADDCONTACT);
		}
	}
}

void SipManager::onCallMenuAboutToHide()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	IMetaTabWindow *window = FCallMenus.key(menu);
	if (menu && window)
	{
		menu->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 0);
		menu->clear();
	}
}

void SipManager::onMetaTabWindowCreated(IMetaTabWindow *AWindow)
{
	if(AWindow->isContactPage())
	{
		QLabel *separator = new QLabel;
		separator->setFixedWidth(12);
		separator->setPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_SEPARATOR)));
		AWindow->toolBarChanger()->insertWidget(separator, TBG_MCMTW_P2P_CALL);

		Menu *callMenu = new Menu(AWindow->toolBarChanger()->toolBar());
		callMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 0);
		connect(callMenu, SIGNAL(aboutToShow()), this, SLOT(onCallMenuAboutToShow()));
		connect(callMenu, SIGNAL(aboutToHide()), this, SLOT(onCallMenuAboutToHide()));

		QToolButton *callButton = AWindow->toolBarChanger()->insertAction(callMenu->menuAction(), TBG_MCMTW_P2P_CALL);
		callButton->setObjectName("tbSipCall");
		callButton->setPopupMode(QToolButton::InstantPopup);
		FCallMenus.insert(AWindow, callMenu);
	}
}

void SipManager::onMetaTabWindowDestroyed(IMetaTabWindow *AWindow)
{
	FCallMenus.remove(AWindow);
}

Q_EXPORT_PLUGIN2(plg_sipmanager, SipManager)
