#include "sipmanager.h"

#include <QLabel>
#include <QProcess>
#include <QPainter>
#include <QTextCodec>

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/sipcallhandlerorders.h>
#include <definitions/gateserviceidentifiers.h>
#include <utils/log.h>
#include <utils/iconstorage.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include <utils/custombordercontainer.h>

#include "sipcall.h"
#include "frameconverter.h"
#include "pjsipdefines.h"
#include "pjsipcallbacks.h"

#include "videocallwindow.h"

#if defined(Q_WS_WIN)
# include <windows.h>
#elif defined(Q_WS_MAC)
# include <utils/macutils.h>
#endif

#define ADR_STREAM_JID           Action::DR_StreamJid
#define ADR_WINDOW_METAID        Action::DR_Parametr1
#define ADR_DESTINATIONS         Action::DR_Parametr2
#define ADR_PHONE_NUMBER         Action::DR_Parametr3
#define ADR_AUTO_START_VIDEO     Action::DR_Parametr4
#define ADR_STATUS_ICON          Action::DR_Parametr1

/* The PJSIP module instance. */
static pjsip_module mod_default_handler =
{
	NULL, NULL,                               /* prev, next.		*/
	{ (char*)"mod-default-handler", 19 },	    /* Name.		*/
	-1,                                       /* Id			*/
	PJSIP_MOD_PRIORITY_APPLICATION,           /* Priority	        */
	NULL,                                     /* load()		*/
	NULL,                                     /* start()		*/
	NULL,                                     /* stop()		*/
	NULL,                                     /* unload()		*/
	NULL,                                     /* on_rx_request()	*/
	NULL,                                     /* on_rx_response()	*/
	NULL,                                     /* on_tx_request.	*/
	NULL,                                     /* on_tx_response()	*/
	NULL,                                     /* on_tsx_state()	*/
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
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FMessageStyles = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FNotifications = NULL;

	inst = this;
	FSHISipQuery = -1;
	FSipStackCreated = false;
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
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

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
			connect(FMetaContacts->instance(), SIGNAL(metaPresenceChanged(IMetaRoster *, const QString &)), SLOT(onMetaPresenceChanged(IMetaRoster *, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams*>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onXmppStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
		}
	}

	connect(FPluginManager->instance(), SIGNAL(shutdownStarted()), SLOT(onShutDownStarted()));

	return FStanzaProcessor;
}

bool SipManager::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipPhone;
		sipPhone.active = true;
		sipPhone.var = NS_RAMBLER_PHONE;
		sipPhone.name = tr("SIP Phone");
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

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_SIPPHONE_MISSEDCALL;
		notifyType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::TabPageNotify|INotification::DockBadge|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_SIPPHONE_MISSEDCALL,notifyType);
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
	if (createSipStack())
	{
#if defined(HAS_VIDEO_SUPPORT) && (HAS_VIDEO_SUPPORT != 0)
		PJCallbacks::registerFrameCallbacks(myframe);
#endif
	}
	return true;
}

bool SipManager::isCallsAvailable() const
{
	return FSipStackCreated && isDevicePresent(ISipDevice::DT_LOCAL_MICROPHONE) && isDevicePresent(ISipDevice::DT_REMOTE_MICROPHONE);
}

bool SipManager::isCallSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery && FDiscovery->discoInfo(AStreamJid, AContactJid).features.contains(NS_RAMBLER_PHONE);
}

ISipCall *SipManager::newCall(const Jid &AStreamJid, const QString &APhoneNumber)
{
	IXmppStream *xmppStream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (xmppStream)
	{
		SipCall *call = new SipCall(this, xmppStream, APhoneNumber, QUuid::createUuid().toString());
		emit sipCallCreated(call);
		return call;
	}
	return NULL;
}

ISipCall *SipManager::newCall(const Jid &AStreamJid, const QList<Jid> &ADestinations)
{
	IXmppStream *xmppStream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (xmppStream)
	{
		SipCall *call = new SipCall(this, FStanzaProcessor, xmppStream, ADestinations, QUuid::createUuid().toString());
		emit sipCallCreated(call);
		return call;
	}
	return NULL;
}

QList<ISipCall*> SipManager::findCalls(const Jid &AStreamJid, const Jid &AContactJid, const QString &ASessionId) const
{
	return SipCall::findCalls(AStreamJid, AContactJid, ASessionId);
}

int SipManager::sipAccountId(const Jid &AStreamJid) const
{
	return FSipAccounts.value(AStreamJid.bare(), -1);
}

bool SipManager::setSipAccountRegistration(const Jid &AStreamJid, bool ARegistered)
{
	if (FSipAccounts.contains(AStreamJid.bare()) != ARegistered)
	{
		IXmppStream *stream = FXmppStreams ? FXmppStreams->xmppStream(AStreamJid) : NULL;
		if (ARegistered && stream!=NULL && isCallsAvailable())
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
			acc_cfg.vid_out_auto_transmit = PJ_FALSE;
			acc_cfg.register_on_acc_add = PJ_FALSE;

			pjsua_acc_id acc_id = -1;
			pj_status_t status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
			if (status == PJ_SUCCESS)
			{
				status = pjsua_acc_set_registration(acc_id, PJ_TRUE);
				if (status == PJ_SUCCESS)
				{
					FSipAccounts.insert(AStreamJid.bare(),acc_id);
					LogDetail(QString("[SipManager]: SIP account '%1' added and registration started, id=%2").arg(AStreamJid.bare()).arg(acc_id));
					return true;
				} // pjsua_acc_set_registration
				else
				{
					LogError(QString("[SipManager]: Failed to start registration of SIP account '%1', status=%2, error=%3").arg(AStreamJid.bare()).arg(status).arg(resolveErrorCode(status)));
				}
			} // pjsua_acc_add
			else
			{
				LogError(QString("[SipManager]: Failed to add SIP account '%1', status=%2, error=%3").arg(AStreamJid.bare()).arg(status).arg(resolveErrorCode(status)));
			}
			if (acc_id != -1)
			{
				pjsua_acc_del(acc_id);
			}
		}
		else if (!ARegistered)
		{
			pjsua_acc_id acc_id = sipAccountId(AStreamJid);
			pj_status_t status = pjsua_acc_set_registration(acc_id,PJ_FALSE);
			if (status == PJ_SUCCESS)
			{
				LogDetail(QString("[SipManager]: Unregistration of SIP account '%1' started").arg(AStreamJid.bare()));
				return true;
			}
			else
			{
				LogError(QString("[SipManager]: Failed to start unregistration of SIP account '%1', status=%2, error=%3").arg(AStreamJid.bare()).arg(status).arg(resolveErrorCode(status)));
			}
		}
		return false;
	}
	return true;
}

bool SipManager::updateAvailDevices()
{
	if (FSipStackCreated)
	{
		FAvailDevices.clear();
	
		pj_status_t status = pjmedia_aud_dev_refresh();
		if (status != PJ_SUCCESS)
		{
			LogError(QString("[SipManager::availDevices]: Failed to refresh audio devices! pjmedia_aud_dev_refresh() returned (%1) %2").arg(status).arg(resolveErrorCode(status)));
		}

		status = pjmedia_vid_dev_refresh();
		if (status != PJ_SUCCESS)
		{
			LogError(QString("[SipManager::availDevices]: Failed to refresh video devices! pjmedia_vid_dev_refresh() returned (%1) %2").arg(status).arg(resolveErrorCode(status)));
		}

		uint numAudDevices = 64;
		pjmedia_aud_dev_info aud_dev_info[64];
		pjsua_enum_aud_devs(aud_dev_info, &numAudDevices);
		for (uint i = 0; i < numAudDevices; i++)
		{
			if (aud_dev_info[i].input_count)
			{
				ISipDevice device;
				device.index = i;
				device.type = ISipDevice::DT_LOCAL_MICROPHONE;
				device.name = QString::fromLocal8Bit(aud_dev_info[i].name);
				FAvailDevices.insertMulti(ISipDevice::DT_LOCAL_MICROPHONE,device);
				LogDetail(QString("[SipManager::availDevices]: Found INPUT audio device with index %1 and name '%2'. Driver is %3").arg(i).arg(device.name).arg(aud_dev_info[i].driver));
			}
			if (aud_dev_info[i].output_count)
			{
				ISipDevice device;
				device.index = i;
				device.type = ISipDevice::DT_REMOTE_MICROPHONE;
				device.name = QString::fromLocal8Bit(aud_dev_info[i].name);
				FAvailDevices.insertMulti(ISipDevice::DT_REMOTE_MICROPHONE,device);
				LogDetail(QString("[SipManager::availDevices]: Found OUTPUT audio device with index %1 and name '%2'. Driver is %3").arg(i).arg(device.name).arg(aud_dev_info[i].driver));
			}
		}

		uint numVidDevices = 64;
		pjmedia_vid_dev_info vid_dev_info[64];
		pjsua_vid_enum_devs(vid_dev_info, &numVidDevices);
		for (uint i = 0; i < numVidDevices; i++)
		{
			if (vid_dev_info[i].fmt_cnt)
			{
#if defined(Q_WS_MAC)
				QString name = convertFromMacCyrillic(vid_dev_info[i].name);
#else
				QString name = QString::fromLocal8Bit(vid_dev_info[i].name);
#endif
				if (vid_dev_info[i].dir & PJMEDIA_DIR_ENCODING)
				{
					ISipDevice device;
					device.index = i;
					device.type = ISipDevice::DT_LOCAL_CAMERA;
					device.name = name;
					FAvailDevices.insertMulti(ISipDevice::DT_LOCAL_CAMERA,device);
					LogDetail(QString("[SipManager::availDevices]: Found INPUT video device with index %1 and name '%2'. Driver is %3").arg(i).arg(name).arg(vid_dev_info[i].driver));
				}
				if (vid_dev_info[i].dir & PJMEDIA_DIR_DECODING)
				{
					ISipDevice device;
					device.index = i;
					device.type = ISipDevice::DT_REMOTE_CAMERA;
					device.name = name;
					FAvailDevices.insertMulti(ISipDevice::DT_REMOTE_CAMERA,device);
					LogDetail(QString("[SipManager::availDevices]: Found OUTPUT video device with index %1 and name '%2'. Driver is %3").arg(i).arg(name).arg(vid_dev_info[i].driver));
				}
			}
		}
		emit availDevicesChanged();
		return true;
	}
	return false;
}

ISipDevice SipManager::findDevice(ISipDevice::Type AType, int ADeviceId) const
{
	for (QMap<int,ISipDevice>::const_iterator it=FAvailDevices.constBegin(); it!=FAvailDevices.constEnd(); it++)
		if (it.key()==AType && it->index==ADeviceId)
			return it.value();
	return ISipDevice();
}

ISipDevice SipManager::findDevice(ISipDevice::Type AType, const QString &AName) const
{
	for (QMap<int,ISipDevice>::const_iterator it=FAvailDevices.constBegin(); it!=FAvailDevices.constEnd(); it++)
		if (it.key()==AType && it->name==AName)
			return it.value();
	return ISipDevice();
}

bool SipManager::isDevicePresent(ISipDevice::Type AType) const
{
	return FAvailDevices.contains(AType);
}

ISipDevice SipManager::preferedDevice(ISipDevice::Type AType) const
{
	QString name; // TODO: Get name for type from options
	ISipDevice device = availDevices(AType).value(0); //findDevice(AType,name);
	
	if (device.isNull() && isDevicePresent(AType))
	{
		device.type = AType;
		if (AType == ISipDevice::DT_LOCAL_CAMERA)
			device.index = DEFAULT_CAP_DEV;
		else if (AType == ISipDevice::DT_REMOTE_CAMERA)
			device.index = DEFAULT_REND_DEV;
		else
			device.index = -1;
		device.name = tr("Default");
	}
	return device;
}

QList<ISipDevice> SipManager::availDevices(ISipDevice::Type AType) const
{
	return FAvailDevices.values(AType);
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
		registerCallNotify(ACall);
		if (SipCall::findCalls().count()==1)
		{
			VideoCallWindow *window = new VideoCallWindow(FPluginManager,ACall);
			connect(window,SIGNAL(chatWindowRequested()),SLOT(onVideoCallChatWindowRequested()));
			WidgetManager::showActivateRaiseWindow(window->window());
			WidgetManager::alignWindow(window->window(),Qt::AlignCenter);
			ACall->startCall();
		}
		else
		{
			ACall->rejectCall(ISipCall::RC_BUSY);
			ACall->destroyCall();
		}
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

QString SipManager::resolveErrorCode(int code)
{
	char errmsg[PJ_ERR_MSG_SIZE];
	pj_strerror(code, errmsg, sizeof(errmsg));
	return QString(errmsg);
}

void SipManager::onSipRegistrationState(int AAccountId)
{
	// TODO: check implementation
	if (FSipAccounts.values().contains(AAccountId))
	{
		Jid accJid = FSipAccounts.key(AAccountId);

		pjsua_acc_info info;
		pjsua_acc_get_info(AAccountId, &info);

		bool registered = info.expires>0;
		if (info.status == PJSIP_SC_OK)
			LogDetail(QString("[SipManager]: SIP account '%1' registration changed, registered=%2").arg(accJid.bare()).arg(registered));
		else
			LogError(QString("[SipManager]: Failed to change SIP account '%1' registration, registered=%2").arg(accJid.bare()).arg(registered));
		emit sipAccountRegistrationChanged(AAccountId,registered);

		if (!registered)
		{
			pj_status_t status = pjsua_acc_del(AAccountId);
			if (status == PJ_SUCCESS)
			{
				LogDetail(QString("[SipManager]: SIP account '%1' removed").arg(accJid.bare()));
				FSipAccounts.remove(accJid);
			}
			else
			{
				LogError(QString("[SipManager]: Failed to remove SIP account '%1'").arg(accJid.bare()));
			}
		}
	}
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
			LogError(QString("[SipManager::onIncomingCall]: Failed to end call! pjsua_call_hangup() returned (%1) %2").arg(status).arg(resolveErrorCode(status)));
	}
}

bool SipManager::createSipStack()
{
	if (!FSipStackCreated)
	{
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
				if (pjsua_get_pjsip_endpt())
				{
					PJCallbacks::registerModuleCallbacks(mod_default_handler);
					status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &mod_default_handler);
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
								status = pjsua_start();
								if (status == PJ_SUCCESS)
								{
									FSipStackCreated = true;
									LogDetail(QString("[SipManager] PJSIP stack created successfully"));
									updateAvailDevices();
									return true;
								} // pjsua_start
								else
								{
									LogError(QString("[SipManager]: Failed to start pjsip instance, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
								}
							} // pjsua_transport_get_info
							else
							{
								LogError(QString("[SipManager]: Failed to get pjsip transport info, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
							}
						} // pjsua_transport_create
						else
						{
							LogError(QString("[SipManager]: Failed to create pjsip UDP transport, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
						}
					} // pjsip_endpt_register_module
					else
					{
						LogError(QString("[SipManager]: Failed to register pjsip module, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
					}
				} // pjsua_get_pjsip_endpt
				else
				{
					LogError(QString("[SipManager]: Failed to get pjsip instance endpoint, status=%2, error=%3").arg(status).arg(resolveErrorCode(status)));
				}
			} // pjsua_init
			else
			{
				LogError(QString("[SipManager]: Failed to init pjsip instance, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
			}
			pjsua_destroy();
		}
		else
		{
			LogError(QString("[SipManager]: Failed to create pjsip instance, status=%1, error=%2").arg(status).arg(resolveErrorCode(status)));
		}
	}
	return FSipStackCreated;
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
	bool handled = false;
	LogDetail(QString("[SipManager]: Processing incoming call from %1 to %2, sid='%3'").arg(AContactJid.full(), AStreamJid.full(), ASessionId));

	IXmppStream *xmppStream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (xmppStream)
	{
		SipCall *call = new SipCall(this,FStanzaProcessor,xmppStream,AContactJid,ASessionId);
		emit sipCallCreated(call);

		for (QMap<int,ISipCallHandler *>::const_iterator it=FCallHandlers.constBegin(); !handled && it!=FCallHandlers.constEnd(); it++)
			handled = it.value()->handleSipCall(it.key(),call);

		if (!handled)
		{
			call->rejectCall(ISipCall::RC_NOHANDLER);
			call->deleteLater();
		}
	}
	
	return handled;
}

void SipManager::registerCallNotify(ISipCall *ACall)
{
	CallNotifyParams &params = FCallNotifyParams[ACall];
	params.rosterNotifyId = -1;

	connect(ACall->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(ACall->instance(),SIGNAL(callDestroyed()),SLOT(onCallDestroyed()));
}

void SipManager::showMissedCallNotify(ISipCall *ACall)
{
	if (FNotifications && FMessageProcessor && FMessageWidgets)
	{
		if (FMessageProcessor->createMessageWindow(ACall->streamJid(),ACall->contactJid(),Message::Chat,IMessageHandler::SM_ASSIGN))
		{
			IChatWindow *window = FMessageWidgets->findChatWindow(ACall->streamJid(),ACall->contactJid());
			if (window)
			{
				INotification notify;
				notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_SIPPHONE_MISSEDCALL);
				if (notify.kinds > 0)
				{
					int missedCount = 1;
					QList<int> oldNotifies = findRelatedNotifies(ACall->streamJid(),ACall->contactJid());
					foreach(int oldNotifyId, oldNotifies)
					{
						INotification oldNotify = FNotifications->notificationById(oldNotifyId);
						missedCount += oldNotify.data.value(NDR_TABPAGE_NOTIFYCOUNT).toInt();
						FNotifications->removeNotification(oldNotifyId);
					}

					QString name = FNotifications->contactName(ACall->streamJid(),ACall->contactJid());
					QString missedcalls = tr("%n missed call(s)","",missedCount);

					notify.typeId = NNT_CHAT_MESSAGE;
					notify.data.insert(NDR_STREAM_JID,ACall->streamJid().full());
					notify.data.insert(NDR_CONTACT_JID,ACall->contactJid().full());
					notify.data.insert(NDR_ICON_KEY,MNI_SIPPHONE_CALL_MISSED);
					notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
					notify.data.insert(NDR_ROSTER_ORDER,RNO_SIPCALL_MISSEDCALL);
					notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
					notify.data.insert(NDR_ROSTER_HOOK_CLICK,true);
					notify.data.insert(NDR_ROSTER_CREATE_INDEX,false);
					notify.data.insert(NDR_ROSTER_FOOTER,missedcalls);
					notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::yellow));
					notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(missedcalls));
					notify.data.insert(NDR_ALERT_WIDGET,(qint64)window->instance());
					notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)window->instance());
					notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
					notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_MISSEDCALL);
					notify.data.insert(NDR_TABPAGE_ICONBLINK,false);
					notify.data.insert(NDR_TABPAGE_TOOLTIP,missedcalls);
					notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,missedCount);
					notify.data.insert(NDR_SOUND_FILE,SDF_SIPPHONE_CALL_MISSED);

					FMissedCallNotifies.insert(FNotifications->appendNotification(notify),window);
					connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()),Qt::UniqueConnection);
				}
			}
		}
	}
}

void SipManager::showNotifyInRoster(ISipCall *ACall, const QString &AIconId, const QString &AFooter)
{
	if (FRostersViewPlugin && FRostersModel)
	{
		CallNotifyParams &params = FCallNotifyParams[ACall];
		FRostersViewPlugin->rostersView()->removeNotify(params.rosterNotifyId);
		params.rosterNotifyId = -1;

		if (!AFooter.isEmpty())
		{
			IRostersNotify notify;
			notify.order = RNO_SIPCALL_CALLSTATE;
			notify.flags = IRostersNotify::AllwaysVisible;
			notify.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AIconId);
			notify.footer = AFooter;

			QList<IRosterIndex *> indexes = FRostersModel->getContactIndexList(ACall->streamJid(),ACall->contactJid(),false);
			if (!indexes.isEmpty() && !AFooter.isEmpty())
				params.rosterNotifyId = FRostersViewPlugin->rostersView()->insertNotify(notify,indexes);
		}
	}
}

void SipManager::showNotifyInChatWindow(ISipCall *ACall,const QString &AIconId, const QString &ANotify, bool AOpen)
{
	if (FMessageProcessor && FMessageWidgets)
	{
		CallNotifyParams &params = FCallNotifyParams[ACall];
		if (FMessageProcessor->createMessageWindow(ACall->streamJid(),ACall->contactJid(),Message::Chat,AOpen ? IMessageHandler::SM_MINIMIZED : IMessageHandler::SM_ASSIGN))
		{
			IChatWindow *window = FMessageWidgets->findChatWindow(ACall->streamJid(),ACall->contactJid());
			if (window)
			{
				if (!params.contentId.isNull())
				{
					IMessageContentOptions options;
					options.action = IMessageContentOptions::Remove;
					options.contentId = params.contentId;
					window->viewWidget()->changeContentHtml(QString::null,options);
				}

				IMessageContentOptions options;
				options.kind = IMessageContentOptions::Status;
				options.type |= IMessageContentOptions::Notification;
				options.direction = IMessageContentOptions::DirectionIn;
				options.time = QDateTime::currentDateTime();
				options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;

				QUrl iconFile = QUrl::fromLocalFile(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(AIconId));
				QString html = QString("<img src='%1'/> <b>%2</b>").arg(iconFile.toString()).arg(Qt::escape(ANotify));

				params.contentId = window->viewWidget()->changeContentHtml(html,options);
				params.view = window->viewWidget();
				params.contentTime = options.time;

				connect(window->viewWidget()->instance(),SIGNAL(contentChanged(const QUuid &, const QString &, const IMessageContentOptions &)),
					SLOT(onViewWidgetContentChanged(const QUuid &, const QString &, const IMessageContentOptions &)),Qt::UniqueConnection);
			}
		}
	}
}

QList<int> SipManager::findRelatedNotifies(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QList<int> notifies;
	IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(AStreamJid) : NULL;
	QString metaId = mroster!=NULL ? mroster->itemMetaContact(AContactJid) : QString::null;
	for (QMap<int, IChatWindow *>::const_iterator it=FMissedCallNotifies.constBegin(); it!=FMissedCallNotifies.constEnd(); it++)
	{
		if (mroster && !metaId.isEmpty())
		{
			if (mroster->streamJid()==(*it)->streamJid() && mroster->itemMetaContact((*it)->contactJid())==metaId)
				notifies.append(it.key());
		}
		else if (AContactJid == (*it)->contactJid())
		{
			notifies.append(it.key());
		}
	}
	return notifies;
}

void SipManager::updateCallButtonStatusIcon(IMetaTabWindow *AWindow) const
{
	if (isCallsAvailable() && FCallMenus.contains(AWindow))
	{
		QImage statusIcon;
		foreach(Jid itemJid, AWindow->metaRoster()->metaContact(AWindow->metaId()).items)
		{
			foreach(IPresenceItem pitem, AWindow->metaRoster()->itemPresences(itemJid)) 
			{
				if (isCallSupported(AWindow->metaRoster()->streamJid(),pitem.itemJid))
				{
					statusIcon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_METACONTACTS_ONLINE_ICON);
					break;
				}
			}
		}

		Menu *callMenu = FCallMenus.value(AWindow);
		callMenu->menuAction()->setData(ADR_STATUS_ICON,statusIcon);
		AWindow->toolBarChanger()->toolBar()->update();
	}
}

bool SipManager::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::Paint)
	{
		QToolButton *button = qobject_cast<QToolButton *>(AObject);
		if (button)
		{
			Action *action = qobject_cast<Action *>(button->defaultAction());
			QImage statusIcon = action!=NULL ? action->data(ADR_STATUS_ICON).value<QImage>() : QImage();
			if (!statusIcon.isNull())
			{
				button->removeEventFilter(this);
				QApplication::sendEvent(button, AEvent);
				button->installEventFilter(this);

				QPainter p(button);
				QSize iconSize = statusIcon.size();
				QRect iconRect = QRect(button->width() - iconSize.width(), button->height() - iconSize.height(), iconSize.width(), iconSize.height());
				p.drawImage(iconRect, statusIcon);
				p.end();

				return true;
			}
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void SipManager::onCallStateChanged(int AState)
{
	ISipCall *call = qobject_cast<ISipCall *>(sender());
	if (call && !call->isDirectCall())
	{
		// Notify in roster
		if (AState==ISipCall::CS_CALLING || AState==ISipCall::CS_CONNECTING)
		{
			if (call->role() == ISipCall::CR_INITIATOR)
				showNotifyInRoster(call,MNI_SIPPHONE_CALL_OUT,tr("Calling to..."));
			else if (call->role() == ISipCall::CR_RESPONDER)
				showNotifyInRoster(call,MNI_SIPPHONE_CALL_IN,tr("Calling you..."));
		}
		else if (AState == ISipCall::CS_TALKING)
		{
			showNotifyInRoster(call,call->role()==ISipCall::CR_INITIATOR ? MNI_SIPPHONE_CALL_OUT : MNI_SIPPHONE_CALL_IN,tr("Call in progress..."));
		}
		else
		{
			showNotifyInRoster(call,QString::null,QString::null);
		}

		// Notify in chat window
		QString userNick = FMessageStyles!=NULL ? FMessageStyles->contactName(call->streamJid(),call->contactJid()) : call->contactJid().uBare();
		if (call->role() == ISipCall::CR_INITIATOR)
		{
			if (AState==ISipCall::CS_CALLING || AState==ISipCall::CS_CONNECTING)
			{
				showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Calling to %1.").arg(userNick));
			}
			else if (AState == ISipCall::CS_TALKING)
			{
				showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call to %1.").arg(userNick));
			}
			else if (AState == ISipCall::CS_FINISHED)
			{
				if (call->callTime() > 0)
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call to %1 finished, duration %2.").arg(userNick,call->callTimeString()));
				else
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call to %1 canceled.").arg(userNick));
			}
			else if (AState == ISipCall::CS_ERROR)
			{
				switch (call->errorCode())
				{
				case ISipCall::EC_BUSY:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("%1 is now talking. Call later.").arg(userNick));
					break;
				case ISipCall::EC_NOTAVAIL:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("%1 could not accept the call.").arg(userNick));
					break;
				case ISipCall::EC_NOANSWER:
				case ISipCall::EC_REJECTED:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("%1 did not accept the call.").arg(userNick));
					break;
				case ISipCall::EC_CONNECTIONERR:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call failed, unable to establish connection with %1.").arg(userNick));
					break;
				default:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call to %1 has failed. Reason: %2.").arg(userNick).arg(call->errorString()));
				}
			}
		}
		else if (call->role() == ISipCall::CR_RESPONDER)
		{
			if (AState==ISipCall::CS_CALLING || AState==ISipCall::CS_CONNECTING)
			{
				showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("%1 calling you.").arg(userNick));
			}
			else if (AState == ISipCall::CS_TALKING)
			{
				showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("Call from %1.").arg(userNick));
			}
			else if (AState == ISipCall::CS_FINISHED)
			{
				if (call->callTime() > 0)
				{
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("Call from %1 finished, duration %2.").arg(userNick,call->callTimeString()));
				}
				else if (call->rejectCode() == ISipCall::RC_EMPTY)
				{
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("Call from %1 accepted.").arg(userNick));
				}
				else if (call->rejectCode() == ISipCall::RC_BUSY)
				{
					showMissedCallNotify(call);
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_MISSED,tr("Missed call."),true);
				}
				else
				{
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("Call from %1 canceled.").arg(userNick));
				}
			}
			else if (AState == ISipCall::CS_ERROR)
			{
				switch (call->errorCode())
				{
				case ISipCall::EC_NOANSWER:
				case ISipCall::EC_REJECTED:
					showMissedCallNotify(call);
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_MISSED,tr("Missed call."),true);
					break;
				case ISipCall::EC_CONNECTIONERR:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_OUT,tr("Call failed, unable to establish connection with %1.").arg(userNick));
					break;
				default:
					showNotifyInChatWindow(call,MNI_SIPPHONE_CALL_IN,tr("Call from %1 has failed. Reason: %2.").arg(userNick).arg(call->errorString()));
				}
			}
		}
	}
}

void SipManager::onCallDestroyed()
{
	ISipCall *call = qobject_cast<ISipCall *>(sender());
	if (call)
	{
		CallNotifyParams params = FCallNotifyParams.take(call);
		if (FRostersViewPlugin)
			FRostersViewPlugin->rostersView()->removeNotify(params.rosterNotifyId);
		FPluginManager->continueShutdown();
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
			registerCallNotify(call);
			VideoCallWindow *window = new VideoCallWindow(FPluginManager,call);
			connect(window,SIGNAL(chatWindowRequested()),SLOT(onVideoCallChatWindowRequested()));
			WidgetManager::showActivateRaiseWindow(window->window());
			WidgetManager::alignWindow(window->window(),Qt::AlignCenter);
			window->sipCall()->setDeviceProperty(ISipDevice::DT_LOCAL_CAMERA,ISipDevice::LCP_AUTO_START,action->data(ADR_AUTO_START_VIDEO).toBool());
			window->sipCall()->startCall();
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

void SipManager::onShutDownStarted()
{
	foreach(ISipCall *call, SipCall::findCalls())
	{
		FPluginManager->delayShutdown();
		call->destroyCall();
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
				QString number = FGateways!=NULL ? FGateways->normalizedContactLogin(FGateways->gateDescriptorById(GSID_SMS),itemJid.uNode()) : itemJid.node();
				phoneNumbers.append(number);
			}
		}

		bool phoneCallEnabled = isCallsAvailable() && SipCall::findCalls().isEmpty();
		foreach(QString number, phoneNumbers)
		{
			Action *phoneCallAction = new Action(menu);
			phoneCallAction->setEnabled(phoneCallEnabled);
			phoneCallAction->setText(FGateways!=NULL ? FGateways->formattedContactLogin(FGateways->gateDescriptorById(GSID_SMS),number) : number);
			phoneCallAction->setData(ADR_STREAM_JID,window->metaRoster()->streamJid().full());
			phoneCallAction->setData(ADR_PHONE_NUMBER,number);
			connect(phoneCallAction,SIGNAL(triggered()),SLOT(onStartPhoneCall()));
			menu->addAction(phoneCallAction,AG_SPCM_SIPPHONE_PHONE_LIST);
		}

		bool videoCallEnabled = isCallsAvailable() && SipCall::findCalls().isEmpty() && !destinations.isEmpty();
		Action *audioCallAction = new Action(menu);
		audioCallAction->setEnabled(videoCallEnabled);
		audioCallAction->setText(tr("Audio Call"));
		audioCallAction->setData(ADR_STREAM_JID,window->metaRoster()->streamJid().full());
		audioCallAction->setData(ADR_DESTINATIONS,destinations);
		audioCallAction->setData(ADR_AUTO_START_VIDEO,false);
		connect(audioCallAction,SIGNAL(triggered()),SLOT(onStartVideoCall()));
		menu->addAction(audioCallAction,AG_SPCM_SIPPHONE_VIDEO_LIST);

		Action *videoCallAction = new Action(menu);
		videoCallAction->setEnabled(videoCallEnabled);
		videoCallAction->setText(tr("Video Call"));
		videoCallAction->setData(ADR_STREAM_JID,window->metaRoster()->streamJid().full());
		videoCallAction->setData(ADR_DESTINATIONS,destinations);
		videoCallAction->setData(ADR_AUTO_START_VIDEO,true);
		connect(videoCallAction,SIGNAL(triggered()),SLOT(onStartVideoCall()));
		menu->addAction(videoCallAction,AG_SPCM_SIPPHONE_VIDEO_LIST);

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

void SipManager::onChatWindowActivated()
{
	if (FNotifications)
		FNotifications->removeNotification(FMissedCallNotifies.key(qobject_cast<IChatWindow *>(sender())));
}

void SipManager::onVideoCallChatWindowRequested()
{
	if (FMessageProcessor)
	{
		VideoCallWindow *window = qobject_cast<VideoCallWindow *>(sender());
		if (window)
			FMessageProcessor->createMessageWindow(window->sipCall()->streamJid(),window->sipCall()->contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
	}
}

void SipManager::onNotificationActivated(int ANotifyId)
{
	if (FMissedCallNotifies.contains(ANotifyId))
	{
		IChatWindow *window = FMissedCallNotifies.value(ANotifyId);
		if (window)
			window->showTabPage();
		FNotifications->removeNotification(ANotifyId);
	}
}

void SipManager::onNotificationRemoved(int ANotifyId)
{
	FMissedCallNotifies.remove(ANotifyId);
}

void SipManager::onXmppStreamRemoved(IXmppStream *AXmppStream)
{
	foreach(ISipCall *call, SipCall::findCalls(AXmppStream->streamJid()))
		call->rejectCall(ISipCall::RC_BYUSER);
	setSipAccountRegistration(AXmppStream->streamJid().bare(),false);
}

void SipManager::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(ADiscoInfo.streamJid) : NULL;
	IMetaTabWindow *window = mroster!=NULL ? FMetaContacts->findMetaTabWindow(mroster->streamJid(),mroster->itemMetaContact(ADiscoInfo.contactJid)) : NULL;
	updateCallButtonStatusIcon(window);
}

void SipManager::onMetaTabWindowCreated(IMetaTabWindow *AWindow)
{
	if(AWindow->isContactPage())
	{
		QLabel *separator = new QLabel;
		separator->setFixedWidth(12);
		separator->setPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_CALL_SEPARATOR)));
		AWindow->toolBarChanger()->insertWidget(separator, TBG_MCMTW_P2P_CALL);

		Menu *callMenu = new Menu(AWindow->toolBarChanger()->toolBar());
		callMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 0);
		callMenu->menuAction()->setToolTip(tr("You can make free calls to other Rambler Contacts users"));
		connect(callMenu, SIGNAL(aboutToShow()), this, SLOT(onCallMenuAboutToShow()));
		connect(callMenu, SIGNAL(aboutToHide()), this, SLOT(onCallMenuAboutToHide()));

		QToolButton *callButton = AWindow->toolBarChanger()->insertAction(callMenu->menuAction(), TBG_MCMTW_P2P_CALL);
		callButton->setObjectName("tbSipCall");
		callButton->setPopupMode(QToolButton::InstantPopup);
		callButton->installEventFilter(this);
		FCallMenus.insert(AWindow, callMenu);
		updateCallButtonStatusIcon(AWindow);
	}
}

void SipManager::onMetaTabWindowDestroyed(IMetaTabWindow *AWindow)
{
	FCallMenus.remove(AWindow);
}

void SipManager::onMetaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId)
{
	IMetaTabWindow *window = FMetaContacts->findMetaTabWindow(AMetaRoster->streamJid(),AMetaId);
	updateCallButtonStatusIcon(window);
}

void SipManager::onViewWidgetContentChanged(const QUuid &AContentId, const QString &AMessage, const IMessageContentOptions &AOptions)
{
	Q_UNUSED(AContentId);
	Q_UNUSED(AMessage);
	if (AOptions.action==IMessageContentOptions::InsertAfter && AOptions.contentId.isNull())
	{
		IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
		for (QMap<ISipCall *, CallNotifyParams>::iterator it = FCallNotifyParams.begin(); it!=FCallNotifyParams.end(); it++)
		{
			if (it->view==widget && it->contentTime<=AOptions.time)
			{
				it->contentId = QUuid();
				break;
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_sipmanager, SipManager)
