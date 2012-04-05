#include "sipmanager.h"

#include "sipcall.h"
#include <definitions/namespaces.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/notificationtypes.h>
#include <utils/log.h>
#include <QProcess>

#if defined (Q_WS_WIN)
# include <windows.h>
#endif

#include "frameconverter.h"

////////////////////////////////////////////////////////////
//                        PJSIP                           //
////////////////////////////////////////////////////////////

#include <pjsua.h>

static pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata);

/* The module instance. */
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
	&default_mod_on_rx_request,		/* on_rx_request()	*/
	NULL,					/* on_rx_response()	*/
	NULL,					/* on_tx_request.	*/
	NULL,					/* on_tx_response()	*/
	NULL,					/* on_tsx_state()	*/
};

// pjsip callbacks

// callbacks for SipCall
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	if (SipCall * call = SipCall::activeCallForId(call_id))
		call->onCallState(call_id, e);
}

static void on_call_media_state(pjsua_call_id call_id)
{
	if (SipCall * call = SipCall::activeCallForId(call_id))
		call->onCallMediaState(call_id);
}

static void on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)
{
	if (SipCall * call = SipCall::activeCallForId(call_id))
		call->onCallTsxState(call_id, tsx, e);
}

static pj_status_t my_put_frame_callback(int call_id, pjmedia_frame *frame, int w, int h, int stride)
{
	if (SipCall * call = SipCall::activeCallForId(call_id))
		return call->onMyPutFrameCallback(call_id, frame, w, h, stride);
	else
		return -1;
}

static pj_status_t my_preview_frame_callback(pjmedia_frame *frame, const char* colormodelName, int w, int h, int stride)
{
	Q_UNUSED(frame)
	Q_UNUSED(colormodelName)
	Q_UNUSED(w)
	Q_UNUSED(h)
	Q_UNUSED(stride)
	//return RSipPhone::instance()->on_my_preview_frame_callback(frame, colormodelName, w, h, stride);
	// TODO: get here call_id too
	return -1;
}

// callbacks for SipManager
static void on_reg_state(pjsua_acc_id acc_id)
{
	SipManager::callbackInstance()->onRegState(acc_id);
}

static void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *info)
{
	SipManager::callbackInstance()->onRegState2(acc_id, info);
}

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	SipManager::callbackInstance()->onIncomingCall(acc_id, call_id, rdata);
}

/**************
 * SipManager *
 **************/

#define SHC_SIP_REQUEST "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SIP_PHONE "']"

SipManager * SipManager::inst = NULL; // callback instance

SipManager::SipManager() :
	QObject(NULL)
{
	inst = this;
}

SipManager::~SipManager()
{
	pjsua_destroy();
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

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		if(FMetaContacts)
		{
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowCreated(IMetaTabWindow*)), SLOT(onMetaTabWindowCreated(IMetaTabWindow*)));
			connect(FMetaContacts->instance(), SIGNAL(metaTabWindowDestroyed(IMetaTabWindow*)), SLOT(onMetaTabWindowDestroyed(IMetaTabWindow*)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{

		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
			connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

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

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onXmppStreamOpened(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onXmppStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onXmppStreamClosed(IXmppStream *)));
	}

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	connect(this, SIGNAL(streamCreated(const QString&)), this, SLOT(onStreamCreated(const QString&)));

	return FStanzaProcessor!=NULL;
}

bool SipManager::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipPhone;
		sipPhone.active = true;
		sipPhone.var = NS_RAMBLER_SIP_PHONE;
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
		shandle.conditions.append(SHC_SIP_REQUEST);
		FSHISipRequest = FStanzaProcessor->insertStanzaHandle(shandle);
	}
	if (FNotifications)
	{
		INotificationType incomingNotifyType;
		incomingNotifyType.order = OWO_NOTIFICATIONS_SIPPHONE;
		incomingNotifyType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::AlertWidget|INotification::ShowMinimized|INotification::TabPageNotify|INotification::DockBadge;
		incomingNotifyType.kindDefs = incomingNotifyType.kindMask;
		FNotifications->registerNotificationType(NNT_SIPPHONE_CALL,incomingNotifyType);

		INotificationType missedNotifyType;
		missedNotifyType.order = OWO_NOTIFICATIONS_SIPPHONE_MISSED;
		missedNotifyType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::AlertWidget|INotification::ShowMinimized|INotification::TabPageNotify|INotification::DockBadge;
		missedNotifyType.kindDefs = incomingNotifyType.kindMask;
		FNotifications->registerNotificationType(NNT_SIPPHONE_MISSEDCALL,missedNotifyType);
	}
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
	return FDiscovery && FDiscovery->discoInfo(AStreamJid, AContactJid).features.contains(NS_RAMBLER_SIP_PHONE);
}

ISipCall *SipManager::newCall()
{
	SipCall * call = new SipCall(ISipCall::CR_INITIATOR, this);
	connect(call, SIGNAL(destroyed(QObject*)), SLOT(onCallDestroyed(QObject*)));
	calls << call;
	return call;
}

QList<ISipCall*> SipManager::findCalls(const Jid &AStreamJid)
{
	if (AStreamJid == Jid::null)
		return calls;
	else
	{
		QList<ISipCall*> found;
		foreach (ISipCall * call, calls)
			if (call->streamJid() == AStreamJid)
				found << call;
		return found;
	}
}

bool SipManager::isRegisteredAtServer(const Jid &AStreamJid) const
{
	Q_UNUSED(AStreamJid)
	// TODO: implementation
	return false;
}

bool SipManager::registerAtServer(const Jid &AStreamJid, const QString &APassword)
{
	Q_UNUSED(AStreamJid)
	Q_UNUSED(APassword)
	// TODO: implementation
	return false;
}

bool SipManager::unregisterAtServer(const Jid &AStreamJid, const QString &APassword)
{
	Q_UNUSED(AStreamJid)
	Q_UNUSED(APassword)
	// TODO: implementation
	return false;
}

QList<ISipDevice> SipManager::availDevices(ISipDevice::Type AType) const
{
	Q_UNUSED(AType)
	// TODO: implementation
	return QList<ISipDevice>();
}

ISipDevice SipManager::getDevice(ISipDevice::Type AType, int ADeviceId) const
{
	Q_UNUSED(AType)
	Q_UNUSED(ADeviceId)
	// TODO: implementation
	return ISipDevice();
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
	handlers.insert(AOrder, AHandler);
}

void SipManager::removeSipCallHandler(int AOrder, ISipCallHandler *AHandler)
{
	if (handlers.value(AOrder, NULL) == AHandler)
		handlers.remove(AOrder);
}

bool SipManager::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISipRequest == AHandleId)
	{
		QDomElement actionElem = AStanza.firstElement("query",NS_RAMBLER_SIP_PHONE).firstChildElement();
		QString sid = actionElem.attribute("sid");
		if (actionElem.tagName() == "open")
		{
			AAccept = true;
			LogDetail(QString("[SipManager]: Incoming call from %1 to %2").arg(AStanza.from(), AStreamJid.full()));

			handleIncomingCall(AStreamJid, AStanza.from());
		}
	}
	return false;
}

bool SipManager::handleIncomingCall(const Jid &AStreamJid, const Jid &AContactJid)
{
	// TODO: check availability of answering the call (busy)
	SipCall * call = new SipCall(ISipCall::CR_RESPONDER, this);
	call->setStreamJid(AStreamJid);
	call->setContactJid(AContactJid);
	bool handled = false;
	foreach (ISipCallHandler * handler, handlers.values())
	{
		if (handled = handler->checkCall(call))
			break;
	}
	if (!handled)
		call->rejectCall(ISipCall::RC_NOHANDLER);
	return handled;
}

void SipManager::onCallDestroyed(QObject * object)
{
	ISipCall * call = qobject_cast<ISipCall*>(object);
	if (call)
	{
		calls.removeAll(call);
		emit sipCallDestroyed(call);
	}
}

SipManager *SipManager::callbackInstance()
{
	return inst;
}

void SipManager::onRegState(int acc_id)
{
	Q_UNUSED(acc_id)
	// TODO: implementation
}

void SipManager::onRegState2(int acc_id, void *info)
{
	Q_UNUSED(acc_id)
	Q_UNUSED(info)
	// TODO: implementation
}

void SipManager::onIncomingCall(int acc_id, int call_id, void *rdata)
{
	Q_UNUSED(acc_id)
	Q_UNUSED(call_id)
	Q_UNUSED(rdata)
	// TODO: implementation
}
