#include "sipphone.h"

#include <winsock2.h>

#include <QProcess>
#include <QMessageBox>
#include <utils/log.h>
#include <utils/iconstorage.h>
#include <utils/custominputdialog.h>
#include <utils/menu.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include "contactselector.h"
#include "sipcallnotifyer.h"

#define SHC_SIP_REQUEST "/iq[@type='set']/query[@xmlns='" NS_RAMBLER_SIP_PHONE "']"

#define ADR_STREAM_JID    Action::DR_StreamJid
#define ADR_CONTACT_JID   Action::DR_Parametr1
#define ADR_STREAM_ID     Action::DR_Parametr2
#define ADR_METAID_WINDOW Action::DR_Parametr3

#define RING_TIMEOUT    30000
#define CLOSE_TIMEOUT   10000

SipPhone::SipPhone()
{
	FGateways = NULL;
	FDiscovery = NULL;
	FMetaContacts = NULL;
	FStanzaProcessor = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FNotifications = NULL;
	FPresencePlugin = NULL;
	FRosterChanger = NULL;
	FMessageStyles = NULL;

	FSHISipRequest = -1;
	FSipPhone = NULL;
	FBackupCallActionMenu = NULL;

	//////FSipPhone = new RSipPhone();
	//////if(FSipPhone->initStack("vsip.rambler.ru", 5065, "rvoip-1@rambler.ru", "a111111"))
	//////{
	//////	FSipPhone->hangup();
	//////	FSipPhone->cleanup();
	//////}
	//////if(FSipPhone->initStack("vsip.rambler.ru", 5065, "rvoip-2@rambler.ru", "a111111"))
	//////{
	//////	FSipPhone->hangup();
	//////	FSipPhone->cleanup();
	//////}
	//////	if(FSipPhone->initStack("vsip.rambler.ru", 5065, "rvoip-1@rambler.ru", "a111111"))
	//////{
	//////	FSipPhone->hangup();
	//////	FSipPhone->cleanup();
	//////}
	//////if(FSipPhone->initStack("vsip.rambler.ru", 5065, "rvoip-2@rambler.ru", "a111111"))
	//////{
	//////	FSipPhone->hangup();
	//////	FSipPhone->cleanup();
	//////}

	connect(this, SIGNAL(streamStateChanged(const QString&, int)), this, SLOT(onStreamStateChanged(const QString&, int)));
}

SipPhone::~SipPhone()
{

}

void SipPhone::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SIP Phone");
	APluginInfo->description = tr("Allows to make voice and video calls over SIP protocol");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Popov S.A.";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SipPhone::initConnections(IPluginManager *APluginManager, int &AInitOrder)
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

bool SipPhone::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipPhone;
		sipPhone.active = true;
		sipPhone.var = NS_RAMBLER_SIP_PHONE;
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


void SipPhone::onXmppStreamOpened(IXmppStream * AXmppStream)
{
	FSipPhone = new RSipPhone(this);
	if(FSipPhone)
	{
		//if(FSipPhone->initStack("vsip.rambler.ru", 5065, AXmppStream->streamJid().eNode(), AXmppStream->password()))
		//if(FSipPhone->initStack("vsip.rambler.ru", 5065, AXmppStream->streamJid().eNode(), "fakepass", AXmppStream->streamJid().pDomain()))
		if(FSipPhone->initStack("vsip.rambler.ru", 5065, AXmppStream->streamJid().eNode(), AXmppStream->password(), AXmppStream->streamJid().pDomain()))
		{
			LogDetail(QString("[SipPhone] SIP stack initialized for '%1'").arg(AXmppStream->streamJid().full()));
			connect(this, SIGNAL(sipSendInvite(const QString&)), FSipPhone, SLOT(call(const QString&)));
			connect(FSipPhone, SIGNAL(signalCallReleased()), this, SLOT(onHangupCall()));
			connect(FSipPhone, SIGNAL(callDeletedProxy(bool)), this, SLOT(sipCallDeletedSlot(bool)));
			connect(FSipPhone, SIGNAL(incomingThreadTimeChange(qint64)), this, SLOT(onIncomingThreadTimeChanged(qint64)));

			//connect(FSipPhone, SIGNAL(signal_DeviceError()), this, SLOT(onSomeInviteError()));
			connect(FSipPhone, SIGNAL(signal_InviteStatus(bool, int, const QString&)), this, SLOT(onInviteStatus(bool, int, const QString&)));

			////connect(ui.btnPreview, SIGNAL(clicked()), _pPhone, SLOT(preview()));
			////connect(ui.btnCall, SIGNAL(clicked()), this, SLOT(call()));
			//connect(ui.btnAccept, SIGNAL(clicked()), _pPhone, SLOT(call()));
			//connect(ui.btnCancel, SIGNAL(clicked()), _pPhone, SLOT(hangup()));
			////connect(quitButton_, SIGNAL(clicked()), this, SLOT(quit()));
			//connect(ui.btnRegister, SIGNAL(clicked()), this, SLOT(registerAccount()));
		}
		else
		{
			LogError(QString("[SipPhone] Failed to initialize SIP stack for '%1'").arg(AXmppStream->streamJid().full()));
		}
	}

	foreach(Action *action, FCallActions.values())
		action->setEnabled(true);
}

void SipPhone::onXmppStreamAboutToClose(IXmppStream *)
{
	foreach(QString sid, FStreams.keys())
		closeStream(sid);
}

void SipPhone::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	foreach(Action *action, FCallActions.values())
		action->setEnabled(false);

	foreach(QString sid, FCallControls.keys())
	{
		RCallControl *pCallControl = FCallControls.take(sid);
		delete pCallControl;
	}

	if(FSipPhone != NULL)
	{
		FSipPhone->hangup();
		FSipPhone->cleanup();
		delete FSipPhone;
		FSipPhone = NULL;
		LogDetail(QString("[SipPhone] SIP stack destroyed for '%1'").arg(AXmppStream->streamJid().full()));
	}
}

void SipPhone::onCallTimerTimeout()
{
	foreach(RCallControl* control, FCallControls.values())
	{
		if (control->status()==RCallControl::Accepted && FStreams.contains(control->getSessionID()))
		{
			ISipStream stream = FStreams.value(control->getSessionID());
			QTime time= QTime(0,0,0).addSecs(stream.startTime.time().secsTo(QTime::currentTime()));
			control->statusTextChange(time.toString("hh:mm:ss"));
			QTimer::singleShot(1000,this,SLOT(onCallTimerTimeout()));
		}
	}
}

void SipPhone::onInviteStatus(bool status, int errorCode, const QString& errorString)
{
	if(status == true)
	{
		ISipStream &stream = FStreams[tmpSid];
		stream.state = ISipStream::SS_OPENED;
		emit streamStateChanged(tmpSid, stream.state);
	}
	else
	{
		FSipPhone->registerAccount(false);
		ISipStream &stream = FStreams[tmpSid];
		//stream.errFlag = ISipStream::EF_DEVERR;
		stream.errFlag = errorCode;
		stream.failReason = errorString;
		//removeIncomingNotify(AStreamId);
		//emit streamStateChanged(AStreamId, stream.state);
		closeStream(tmpSid);
		
	}

}

//void SipPhone::onSomeInviteError()
//{
//	ISipStream &stream = FStreams[tmpSid];
//	stream.errFlag = ISipStream::EF_DEVERR;
//	stream.failReason = "Some device error.";
//	//removeIncomingNotify(AStreamId);
//	//emit streamStateChanged(AStreamId, stream.state);
//	closeStream(tmpSid);
//}

void SipPhone::onMetaTabWindowCreated(IMetaTabWindow* iMetaTabWindow)
{
	// Далее добавляем кнопку звонка в tbChanger
	if(iMetaTabWindow->isContactPage())
	{
		ToolBarChanger *tbChanger = iMetaTabWindow->toolBarChanger();
		Jid streamJid = iMetaTabWindow->metaRoster()->streamJid();
		
		Action* callAction = new Action(tbChanger);
		callAction->setText(tr("Call"));
		callAction->setCheckable(true);
		callAction->setEnabled(FSipPhone!=NULL);
		callAction->setData(ADR_METAID_WINDOW, iMetaTabWindow->metaId());
		callAction->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON);
		connect(callAction, SIGNAL(triggered(bool)), SLOT(onCallActionTriggered(bool)));

		QLabel *separator = new QLabel;
		separator->setFixedWidth(12);
		separator->setPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_SEPARATOR)));
		tbChanger->insertWidget(separator, TBG_MCMTW_P2P_CALL);

		QToolButton *btn = tbChanger->insertAction(callAction, TBG_MCMTW_P2P_CALL);
		btn->setObjectName("tbSipCall");
		btn->setPopupMode(QToolButton::InstantPopup);

		Menu* contactsMenu = new Menu(btn);
		contactsMenu->menuAction()->setData(ADR_METAID_WINDOW, iMetaTabWindow->metaId());
		contactsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON);
		connect(contactsMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowContactMenu()));
		connect(contactsMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideContactMenu()));
		callAction->setMenu(contactsMenu);

		// Сохраняем указатель на кнопку. Понадобится для работы с ней. (изменение состояния при открытии/закрытии панели звонков программно)
		FCallActions.insert(iMetaTabWindow->metaId(), callAction);
	}
}

void SipPhone::onMetaTabWindowDestroyed(IMetaTabWindow* AMetaWindow)
{
	if (AMetaWindow)
	{
		QString metaId = AMetaWindow->metaId();
		if (FCallControls.contains(metaId))
			closeStream(FCallControls.value(metaId)->getSessionID());
		FCallActions.remove(metaId);
		FCallControls.remove(metaId);
	}
}

void SipPhone::onChatWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	removeMissedNotify(window);
}

void SipPhone::onChatWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	removeMissedNotify(window);
}

void SipPhone::onAboutToShowContactMenu()
{
	Menu *contactsMenu = qobject_cast<Menu*>(sender());
	QString metaId = contactsMenu!=NULL ? contactsMenu->menuAction()->data(ADR_METAID_WINDOW).toString() : QString::null;

	Action *callAction = FCallActions.value(metaId);
	if (callAction)
		callAction->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 1);

	IMetaTabWindow *metaWindow = findMetaWindow(metaId);
	IMetaRoster* mroster = metaWindow!=NULL ? metaWindow->metaRoster() : NULL;
	if(mroster!=NULL && mroster->isEnabled())
	{
		IMetaContact mcontact = mroster->metaContact(metaId);
		foreach(Jid contactJid, mcontact.items)
		{
			QList<IPresenceItem> pitems = mroster->itemPresences(contactJid);
			foreach(IPresenceItem pitem, pitems)
			{
				if (isSupported(mroster->streamJid(),pitem.itemJid))
				{
					Action *contactAction = new Action(contactsMenu);
					QString contactId = FGateways!=NULL ? FGateways->legacyIdFromUserJid(mroster->streamJid(),contactJid) : contactJid.bare();
					contactAction->setText(pitems.count()>1 ? contactId+"/"+pitem.itemJid.resource() : contactId);
					contactAction->setData(ADR_STREAM_JID,mroster->streamJid().full());
					contactAction->setData(ADR_CONTACT_JID,pitem.itemJid.full());
					contactAction->setData(ADR_METAID_WINDOW,metaId);
					connect(contactAction, SIGNAL(triggered(bool)), SLOT(onStartCallToContact()));
					contactsMenu->addAction(contactAction, AG_SPCM_SIPPHONE_BASECONTACT);
				}
			}
		}

		Action *addContactAction = new Action(contactsMenu);
		addContactAction->setText(tr("Add Contact"));
		addContactAction->setData(ADR_STREAM_JID, mroster->streamJid().full());
		addContactAction->setData(ADR_METAID_WINDOW, metaId);
		connect(addContactAction, SIGNAL(triggered(bool)), SLOT(onShowAddContactDialog()));
		contactsMenu->addAction(addContactAction, AG_SPCM_SIPPHONE_BASECONTACT + 1);
	}
}

void SipPhone::onAboutToHideContactMenu()
{
	Menu *contactsMenu = qobject_cast<Menu*>(sender());
	if (contactsMenu)
	{
		QString metaId = contactsMenu->menuAction()->data(ADR_METAID_WINDOW).toString();
		Action *action = FCallActions.value(metaId);
		if (action)
			action->setIcon(RSR_STORAGE_MENUICONS, MNI_SIPPHONE_CALL_BUTTON, 0);
		contactsMenu->clear();
	}
}

void SipPhone::onShowAddContactDialog()
{
	Action *addContactAction = qobject_cast<Action*>(sender());
	if (FRosterChanger && addContactAction)
	{
		Jid streamJid = addContactAction->data(ADR_STREAM_JID).toString();
		QString metaId = addContactAction->data(ADR_METAID_WINDOW).toString();

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
				IMetaRoster* iMetaRoster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(streamJid) : NULL;
				if (iMetaRoster)
				{
					IMetaContact contact = iMetaRoster->metaContact(metaId);
					dialog->setGroup(contact.groups.toList().value(0));
					dialog->setParentMetaContactId(metaId);
				}
			}
		}
	}
}

void SipPhone::onStartCallToContact()
{
	if (FStreams.isEmpty())
	{
		Action *contactAction = qobject_cast<Action*>(sender());
		QString metaId = contactAction!=NULL ? contactAction->data(ADR_METAID_WINDOW).toString() : QString::null;
		IMetaTabWindow *metaWindow = findMetaWindow(metaId);
		if (metaWindow)
		{
			if(!FCallControls.contains(metaId))
			{
				newRCallControl(QString::null,RCallControl::Caller,metaWindow);
			}
			else
			{
				RCallControl* pCallControl = FCallControls.value(metaId);
				if(pCallControl)
					pCallControl->callStatusChange(RCallControl::Register);
			}
			openStream(metaWindow->metaRoster()->streamJid(), contactAction->data(ADR_CONTACT_JID).toString());
		}
		else
		{
			LogError(QString("[SipPhone] Failed to find meta-window for meta-contact '%1'").arg(metaId));
			CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
			dialog->setDeleteOnClose(true);
			dialog->setCaptionText(tr("Call failure"));
			dialog->setInfoText(tr("Calls NOT supported for current contact"));
			dialog->setAcceptButtonText(tr("Ok"));
			dialog->show();
		}
	}
}

void SipPhone::onCloseCallControl(bool)
{
	RCallControl* pCallControl = qobject_cast<RCallControl*>(sender());
	if(pCallControl)
	{
		QString metaId = pCallControl->getMetaId();
		if (FCallActions.contains(metaId))
		{
			FCallActions[metaId]->setChecked(false);
			if(FBackupCallActionMenu != NULL)
			{
				FCallActions[metaId]->setMenu(FBackupCallActionMenu);
				FBackupCallActionMenu = NULL;
			}
		}

		IMetaTabWindow *metaWindow = findMetaWindow(metaId);
		if (metaWindow)
		{
			metaWindow->removeTopWidget(pCallControl);
		}

		if(FSipPhone)
		{
			FSipPhone->hangup();
			FSipPhone->registerAccount(false);
		}


		closeStream(pCallControl->getSessionID());
		FCallControls.remove(metaId);
	}
}

void SipPhone::onCallActionTriggered(bool status)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString metaId = action->data(ADR_METAID_WINDOW).toString();
		if (status && FCallActions.contains(metaId))
		{
			FCallActions[metaId]->setChecked(true);
			if(FBackupCallActionMenu == NULL)
			{
				FBackupCallActionMenu = FCallActions[metaId]->menu();
				FCallActions[metaId]->setMenu(NULL);
			}
		}
		else if(!status && FCallControls.contains(metaId))
		{
			RCallControl* pCallControl = FCallControls.value(metaId);
			closeStream(pCallControl->getSessionID());
			pCallControl->deleteLater();
		}
	}
}

bool SipPhone::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISipRequest == AHandleId)
	{
		QDomElement actionElem = AStanza.firstElement("query",NS_RAMBLER_SIP_PHONE).firstChildElement();
		QString sid = actionElem.attribute("sid");
		if (actionElem.tagName() == "open")
		{
			AAccept = true;
			// Здесь проверяем возможность установки соединения
			if (FStreams.contains(sid))
			{
				LogError(QString("[SipPhone] Duplicate session id request received from '%1', sid='%2'").arg(AStanza.from()).arg(sid));
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,ErrorHandler(ErrorHandler::CONFLICT));
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else if (!findStream(AStreamJid,AStanza.from()).isEmpty())
			{
				LogError(QString("[SipPhone] Second session request received from '%1', sid='%2'").arg(AStanza.from()).arg(sid));
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,ErrorHandler(ErrorHandler::NOT_ACCEPTABLE));
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
			else
			{
				LogDetail(QString("[SipPhone] Incoming call request received from '%1', sid='%2'").arg(AStanza.from()).arg(sid));
				//Здесь все проверки пройдены, заводим сессию и уведомляем пользователя о входящем звонке
				ISipStream stream;
				stream.sid = sid;
				stream.streamJid = AStreamJid;
				stream.contactJid = AStanza.from();
				stream.kind = ISipStream::SK_RESPONDER;
				stream.state = ISipStream::SS_OPEN;
				stream.timeout = true;
				FStreams.insert(sid,stream);
				FPendingRequests.insert(sid,AStanza.id());
				insertIncomingNotify(stream);
				showCallControlTab(sid);
				emit streamCreated(sid);
				emit streamStateChanged(sid,stream.state);
			}
		}
		else if (actionElem.tagName() == "close")
		{
			LogDetail(QString("[SipPhone] Close call request received from '%1', sid='%2'").arg(AStanza.from()).arg(sid));
			AAccept = true;
			FPendingRequests.insert(sid, AStanza.id());
			// Тут надо добавить в ISipStream причину закрытия, если есть и код ошибки.
			ISipStream &stream = FStreams[sid];
			if(actionElem.hasAttribute("reason"))
				stream.failReason = actionElem.attribute("reason");
			
			closeStream(sid);
		}
	}
	return false;
}

void SipPhone::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FOpenRequests.contains(AStanza.id()))
	{
		QString sid = FOpenRequests.take(AStanza.id());
		QDomElement actionElem = AStanza.firstElement("query",NS_RAMBLER_SIP_PHONE).firstChildElement();
		if (AStanza.type() == "result")
		{
			if (actionElem.tagName()=="opened" && actionElem.attribute("sid")==sid)
			{
				LogDetail(QString("[SipPhone] Call accepted by '%1', sid='%2', id='%3'").arg(AStanza.from()).arg(sid).arg(AStanza.id()));
				tmpSid = sid;
				// Удаленный пользователь принял звонок, устанавливаем соединение
				// Для протокола SIP это означает следующие действия в этом месте:
				// -1) Регистрация на сарвере SIP уже должна быть выполнена!
				// 1) Отправка запроса INVITE
				Jid juri(AStanza.from());
				//QString uri = Jid(AStanza.from()).pBare();
				//QString uri = Jid(AStanza.from()).eNode();
				QString uri = juri.eNode() + "@" + juri.pDomain();
				//QString uri = juri.eNode() + "@vsip.rambler.ru";
				emit sipSendInvite(uri);
				// 2) Получение акцепта на запрос INVITE
				// 3) Установка соединения
				//////////ISipStream &stream = FStreams[sid];
				//////////stream.state = ISipStream::SS_OPENED;
				//////////emit streamStateChanged(sid, stream.state);
			}
			else if (actionElem.tagName()=="closed" && actionElem.attribute("sid")==sid)
			{
				LogDetail(QString("[SipPhone] Call rejected by '%1', sid='%2', id='%3', reason='%4'").arg(AStanza.from()).arg(sid).arg(AStanza.id()).arg(actionElem.attribute("reason")));
				if ( actionElem.hasAttribute("reason"))
					FStreams[sid].failReason = actionElem.attribute("reason");
				removeStream(sid);
			}
			else if (actionElem.tagName()=="close" && actionElem.attribute("sid")==sid && actionElem.hasAttribute("reason"))
			{
				ISipStream& stream = FStreams[sid];
				stream.failReason = actionElem.attribute("reason");
				//removeStream(sid);
			}
			else // Пользователь отказался принимать звонок
			{
				LogError(QString("[SipPhone] Unexpected response received from '%1', sid='%2', id='%3'").arg(AStanza.from()).arg(sid).arg(AStanza.id()));
				removeStream(sid);
			}
		}
		else
		{
			// Получили ошибку, по её коду можно определить причину, уведомляем пользоователя в окне звонка и закрываем сессию
			ErrorHandler err(AStanza.element());
			if (err.code() == ErrorHandler::REQUEST_TIMEOUT)
			{
				LogError(QString("[SipPhone] Open session timeout received from '%1', sid='%2', id='3'").arg(AStanza.from()).arg(sid).arg(AStanza.id()));
				// Если нет ответа от принимающей стороны, то устанавливаем соответствующий флаг и зкрываем соединение
				ISipStream& stream = FStreams[sid];
				stream.timeout = true;
				closeStream(sid);
			}
			else
			{
				LogError(QString("[SipPhone] Failed to open session with '%1', sid='%2', id='3', error='4'").arg(AStanza.from()).arg(sid).arg(AStanza.id()).arg(ErrorHandler(AStanza.element()).message()));
				removeStream(sid);
			}
		}
	}
	else if (FCloseRequests.contains(AStanza.id()))
	{
		// Получили ответ на закрытие сессии, есть ошибка или нет уже не важно
		QString sid = FCloseRequests.take(AStanza.id());
		LogDetail(QString("[SipPhone] Close response received from '%1', sid='%2', id='%3'").arg(AStanza.from()).arg(sid).arg(AStanza.id()));
		removeStream(sid);
	}
}

void SipPhone::onStreamCreated(const QString& sid)
{
	FStreamId = sid;
	if(FStreams.contains(sid))
	{
		ISipStream stream = FStreams.value(sid);
		QString metaId = findMetaId(stream.streamJid, stream.contactJid);
		RCallControl* pCallControl = FCallControls.value(metaId);
		if(pCallControl)
		{
			pCallControl->setSessionId(sid);
			pCallControl->callStatusChange(RCallControl::Ringing);
		}
		LogDetail(QString("[SipPhone] SIP stream sid='%1' created with '%2'").arg(sid).arg(stream.contactJid.full()));
	}
}

bool SipPhone::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery==NULL || FDiscovery->discoInfo(AStreamJid, AContactJid).features.contains(NS_RAMBLER_SIP_PHONE);
}

bool SipPhone::isSupported(const Jid &AStreamJid, const QString &AMetaId) const
{
	IMetaRoster* mroster = FMetaContacts->findMetaRoster(AStreamJid);
	if(mroster!=NULL && mroster->isEnabled())
	{
		IMetaContact metaContact = mroster->metaContact(AMetaId);
		foreach(Jid contactJid, metaContact.items)
		{
			foreach(IPresenceItem pItem, mroster->itemPresences(contactJid))
			{
				if(isSupported(AStreamJid, pItem.itemJid))
					return true;
			}
		}
	}
	return false;
}

QList<QString> SipPhone::streams() const
{
	return FStreams.keys();
}

ISipStream SipPhone::streamById(const QString &AStreamId) const
{
	return FStreams.value(AStreamId);
}

QString SipPhone::findStream(const Jid &AStreamJid, const Jid &AContactJid) const
{
	for (QMap<QString, ISipStream>::const_iterator it=FStreams.constBegin(); it!=FStreams.constEnd(); it++)
	{
		if (it->streamJid==AStreamJid && it->contactJid==AContactJid)
			return it->sid;
	}
	return QString::null;
}

// Отмена звонка пользователем инициатором
void SipPhone::onAbortCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if (pCallControl)
	{
		QString streamId = pCallControl->getSessionID();
		LogDetail(QString("[SipPhone] Abort call requested by user, sid='%1'").arg(streamId));
		if(!streamId.isEmpty())
		{
			if (FStreams.contains(FStreamId))
				FStreams[FStreamId].timeout = false;
			closeStream(streamId);
		}
		else
		{
			emit sipSendUnRegister();
			FSipPhone->registerAccount(false);
			pCallControl->deleteLater();
		}
	}
}

void SipPhone::onAcceptCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if(pCallControl)
	{
		LogDetail(QString("[SipPhone] Accept call requested by user, sid='%1'").arg(pCallControl->getSessionID()));
		acceptStream(pCallControl->getSessionID());
	}
}

void SipPhone::onRedialCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if(pCallControl)
	{
		LogDetail(QString("[SipPhone] Redial requested by user, sid='%1'").arg(pCallControl->getSessionID()));
		Jid contactJidFull = getContactWithPresence(pCallControl->getStreamJid(), pCallControl->getMetaId());
		if(contactJidFull.isValid() && !contactJidFull.isEmpty())
		{
			pCallControl->callStatusChange(RCallControl::Ringing);
			openStream(pCallControl->getStreamJid(), contactJidFull);
		}
	}
}

void SipPhone::onCallbackCall()
{
	RCallControl *pCallControl = qobject_cast<RCallControl *>(sender());
	if(pCallControl)
	{
		LogDetail(QString("[SipPhone] Callback requested by user, sid='%1'").arg(pCallControl->getSessionID()));
		Jid contactJidFull = getContactWithPresence(pCallControl->getStreamJid(), pCallControl->getMetaId());
		if(contactJidFull.isValid() && !contactJidFull.isEmpty())
		{
			pCallControl->callSideChange(RCallControl::Caller);
			pCallControl->callStatusChange(RCallControl::Ringing);
			openStream(pCallControl->getStreamJid(), contactJidFull);
		}
	}
}

void SipPhone::onHangupCall()
{
	LogDetail(QString("[SipPhone] Hangup call, sid='%1'").arg(FStreamId));
	if (FStreams.contains(FStreamId))
		FStreams[FStreamId].timeout = false;
	closeStream(FStreamId);
}

void SipPhone::onStreamStateChanged(const QString& sid, int state)
{
	LogDetail(QString("[SipPhone] Stream state changed, sid='%1', state='%2'").arg(FStreamId).arg(state));

	if(!FStreams.contains(sid))
		return;

	ISipStream &stream = FStreams[sid];
	QString metaId = findMetaId(stream.streamJid, stream.contactJid);

	if(metaId.isEmpty())
		return;

	if(!FCallControls.contains(metaId))
		return;

	RCallControl* pCallControl = FCallControls.value(metaId);
	if(pCallControl == NULL)
		return;

	QTime callTime;
	if (stream.startTime.isValid())
		callTime = QTime(0,0,0).addSecs(stream.startTime.secsTo(QDateTime::currentDateTime()));

	QString userNick = FMessageStyles!=NULL ? FMessageStyles->contactName(stream.streamJid,stream.contactJid) : stream.contactJid.bare();

	if(pCallControl->side() == RCallControl::Caller)
	{
		if(state == ISipStream::SS_OPEN)
		{
			FSipPhone->setCallerName(userNick);
			showNotifyInChatWindow(sid,tr("Calling to %1.").arg(userNick));
		}
		else if(state == ISipStream::SS_OPENED)
		{
			stream.startTime = QDateTime::currentDateTime();
			pCallControl->callStatusChange(RCallControl::Accepted);
			showNotifyInChatWindow(sid,tr("Call to %1.").arg(userNick));
			QTimer::singleShot(1000,this,SLOT(onCallTimerTimeout()));
		}
		else if(state == ISipStream::SS_CLOSE) // Хотим повесить трубку
		{
			// Если нет ответа за таймаут от принимающей стороны, то не закрываем панель,
			// устанавливаем соответствующий статус для возможности совершения повторного вызова
			if(stream.timeout)
			{
				pCallControl->callStatusChange(RCallControl::RingTimeout);
				showNotifyInChatWindow(sid,tr("%1 is not responding.").arg(userNick));
			}
			else if(stream.errFlag != 0)
			{
				showNotifyInChatWindow(sid,tr("Call to %1 has failed. Reason: %2").arg(userNick).arg(stream.failReason), MNI_SIPPHONE_CALL_HANGUP);
				pCallControl->deleteLater();
			}
			else
			{
				if (callTime.isValid())
					showNotifyInChatWindow(sid,tr("Call to %1 finished, duration %2.").arg(userNick).arg(callTime.toString()));
				else
					showNotifyInChatWindow(sid,tr("Call to %1 canceled.").arg(userNick));
				pCallControl->deleteLater();
			}
		}
		else if(state == ISipStream::SS_CLOSED) // Удаленный пользователь повесил трубку
		{
			if(stream.errFlag == ISipStream::EF_REGFAIL)
			{
				showNotifyInChatWindow(sid, tr("Registration on SIP server has failed."), MNI_SIPPHONE_CALL_HANGUP);
				if (FStreams.contains(sid))
				{
					FStreams.remove(sid);
				}
				// Скрываем панель звонка
				pCallControl->deleteLater();
				return;
			}
			if(pCallControl->status() == RCallControl::Ringing) 
			{
				pCallControl->callStatusChange(RCallControl::Hangup); // Говорим что пользователь не захотел брать трубку.
				showNotifyInChatWindow(sid,tr("%1 is not responding.").arg(userNick));
				if(!stream.failReason.isEmpty())
				{
					showNotifyInChatWindow(sid, tr("%1 is not responding. Reason: %2").arg(userNick).arg(stream.failReason));
				}
				else
					showNotifyInChatWindow(sid,tr("%1 is not responding.").arg(userNick));
			}
			else if(pCallControl->status() == RCallControl::Accepted) 
			{
				pCallControl->deleteLater(); // Удаленный пользователь повесил трубку во время разговора. Нам тоже надо.
				showNotifyInChatWindow(sid,tr("Call to %1 finished, duration %2.").arg(userNick).arg(callTime.toString()));
			}
		}
	}
	else if(pCallControl->side() == RCallControl::Receiver)
	{
		if(state == ISipStream::SS_OPEN)
		{
			FSipPhone->setCallerName(userNick);
			showNotifyInChatWindow(sid,tr("%1 calling you.").arg(userNick));
		}
		else if(state == ISipStream::SS_OPENED)
		{
			stream.startTime = QDateTime::currentDateTime();
			pCallControl->callStatusChange(RCallControl::Accepted);
			showNotifyInChatWindow(sid,tr("Call from %1.").arg(userNick));
			QTimer::singleShot(1000,this,SLOT(onCallTimerTimeout()));
		}
		else if(state == ISipStream::SS_CLOSE || state == ISipStream::SS_CLOSED)
		{
			if(stream.timeout)
			{
				if (state == ISipStream::SS_CLOSED)
				{
					pCallControl->callStatusChange(RCallControl::RingTimeout);
					showNotifyInChatWindow(sid,tr("Missed call from %1.").arg(userNick),MNI_SIPPHONE_CALL_MISSED);
					insertMissedNotify(stream);
				}
			}

			else
			{
				if (callTime.isValid())
					showNotifyInChatWindow(sid,tr("Call from %1 finished, duration %2.").arg(userNick).arg(callTime.toString()));
				else
					showNotifyInChatWindow(sid,tr("Call from %1 canceled.").arg(userNick));
				pCallControl->deleteLater();
			}
			if(stream.errFlag == ISipStream::EF_REGFAIL)
			{
				showNotifyInChatWindow(sid, tr("Registration on SIP server has failed."), MNI_SIPPHONE_CALL_HANGUP);
			}
			if(!stream.failReason.isEmpty())
			{
				showNotifyInChatWindow(sid, tr("Call abort by caller side! Reason: %1").arg(stream.failReason), MNI_SIPPHONE_CALL_HANGUP);
			}
		}
	}
}

QString SipPhone::openStream(const Jid &AStreamJid, const Jid &AContactJid)
{
	//////////connect(this, SIGNAL(sipSendRegisterAsInitiator(const Jid&,const Jid&)),
	//////////	FSipPhoneProxy, SLOT(makeRegisterProxySlot(const Jid&, const Jid&)));
	//////////connect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const Jid&, const Jid&)),
	//////////	this, SLOT(sipActionAfterRegistrationAsInitiator(bool, const Jid&, const Jid&)));

	//////////emit sipSendRegisterAsInitiator(AStreamJid, AContactJid);

	QString sid = QUuid::createUuid().toString();
	ISipStream stream;
	stream.sid = sid;
	stream.streamJid = AStreamJid;
	stream.contactJid = AContactJid;
	stream.kind = ISipStream::SK_INITIATOR;
	stream.state = ISipStream::SS_OPEN;
	FStreams.insert(sid,stream);

	tmpSid = sid;
	tempStreamJid = AStreamJid;
	tempContactJid = AContactJid;

	LogDetail(QString("[SipPhone] Opening SIP stream with '%1', sid='%2'").arg(AContactJid.full()).arg(sid));

	//connect(FSipPhone, SIGNAL(signalRegistrationStatusChanged(bool)), this, SLOT(sipActionAfterRegistrationAsInitiator(bool, const Jid&, const Jid&)));
	connect(FSipPhone, SIGNAL(signalRegistrationStatusChanged(bool)), this, SLOT(sipActionAfterRegistrationAsInitiator(bool)));
	FSipPhone->registerAccount(true);

	return QString::null;
}

void SipPhone::sipActionAfterRegistrationAsInitiator(bool ARegistrationResult/*, const Jid& NOT_AStreamJid, const Jid& NOT_AContactJid*/)
{
	//////////disconnect(this, SIGNAL(sipSendRegisterAsInitiator(const Jid&,const Jid&)), 0, 0);
	//////////disconnect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const Jid&, const Jid&)), 0, 0);

	disconnect(FSipPhone, SIGNAL(signalRegistrationStatusChanged(bool)), 0, 0);

	Jid AStreamJid = tempStreamJid;
	Jid AContactJid = tempContactJid;

	QString sid = tmpSid;

	if(ARegistrationResult)
	{
		LogDetail(QString("[SipPhone] Registration on SIP server as initiator is completed, sid='%1'").arg(sid));

		Stanza open("iq");
		open.setType("set").setId(FStanzaProcessor->newId()).setTo(AContactJid.eFull());
		QDomElement openElem = open.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(open.createElement("open")).toElement();

		openElem.setAttribute("sid",sid);

		// Переводим панель в режим Ringing
		QString metaId = findMetaId(AStreamJid, AContactJid);
		if(FCallControls.contains(metaId))
		{
			FCallControls[metaId]->setSessionId(sid);
			FCallControls[metaId]->callStatusChange(RCallControl::Ringing );
		}

		// Открываем вкладку контакта
		if (FMessageProcessor)
			FMessageProcessor->createMessageWindow(AStreamJid,AContactJid,Message::Chat,IMessageHandler::SM_SHOW);

		if (FStanzaProcessor->sendStanzaRequest(this, AStreamJid, open, RING_TIMEOUT))
		{
			LogDetail(QString("[SipPhone] Open request sent to '%1', sid='%2', id='%3'").arg(AContactJid.full()).arg(sid).arg(open.id()));
			ISipStream &stream = FStreams[sid];
			FOpenRequests.insert(open.id(),sid);
			emit streamCreated(sid);
			emit streamStateChanged(sid,stream.state);
		}
		else
		{
			LogError(QString("[SipPhone] Failed to send open request to '%1', sid='%2'").arg(AContactJid.full()).arg(sid));
		}
	}
	else
	{
		LogError(QString("[SipPhone] Failed to register on SIP server as initiator, sid='%1'").arg(sid));
		ISipStream &stream = FStreams[sid];
		stream.state = ISipStream::SS_CLOSED;
		stream.errFlag = ISipStream::EF_REGFAIL;
		emit streamStateChanged(sid, stream.state);
	}
}

// Responder part
bool SipPhone::acceptStream(const QString &AStreamId)
{
	// ПОДКЛЮЧЕНИЕ SIP
	if (FPendingRequests.contains(AStreamId))
	{
		LogDetail(QString("[SipPhone] Accepting SIP stream, sid='%1'").arg(AStreamId));

		tempStreamId = AStreamId;
		connect(FSipPhone, SIGNAL(signalRegistrationStatusChanged(bool)), this, SLOT(sipActionAfterRegistrationAsResponder(bool)));

		// Переводим панель в режим Register
		ISipStream &stream = FStreams[AStreamId];
		stream.timeout = false;

		QString metaId = findMetaId(stream.streamJid, stream.contactJid);
		if(FCallControls.contains(metaId))
		{
			RCallControl* pCallControl = FCallControls[metaId];
			pCallControl->callStatusChange(RCallControl::Register);
		}

		FSipPhone->registerAccount(true);
		return true;
	}
	return false;
}

void SipPhone::sipActionAfterRegistrationAsResponder(bool ARegistrationResult/*, const QString &NOT_AStreamId*/)
{
	//disconnect(this, SIGNAL(sipSendRegisterAsResponder(const QString&)), 0, 0);
	//disconnect(FSipPhoneProxy, SIGNAL(registrationStatusIs(bool, const QString&)), 0, 0);

	disconnect(FSipPhone, SIGNAL(signalRegistrationStatusChanged(bool)), 0, 0);

	QString sid = tempStreamId;

	if(ARegistrationResult)
	{
		LogDetail(QString("[SipPhone] Registration on SIP server as responder is completed, sid='%1'").arg(sid));

		ISipStream &stream = FStreams[sid];

		Stanza opened("iq");
		opened.setType("result").setId(FPendingRequests.value(sid)).setTo(stream.contactJid.eFull());
		QDomElement openedElem = opened.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(opened.createElement("opened")).toElement();
		openedElem.setAttribute("sid",sid);

		// Открываем вкладку контакта
		if (FMessageProcessor)
			FMessageProcessor->createMessageWindow(stream.streamJid,stream.contactJid,Message::Chat,IMessageHandler::SM_SHOW);

		if (FStanzaProcessor->sendStanzaOut(stream.streamJid,opened))
		{
			FPendingRequests.remove(sid);
			stream.state = ISipStream::SS_OPENED;
			removeIncomingNotify(sid);
			emit streamStateChanged(sid, stream.state);
		}
	}
	else
	{
		LogError(QString("[SipPhone] Failed to register on SIP server as responder, sid='%1'").arg(sid));
		ISipStream &stream = FStreams[sid];
		stream.errFlag = ISipStream::EF_REGFAIL;
		//removeIncomingNotify(AStreamId);
		//emit streamStateChanged(AStreamId, stream.state);
		closeStream(sid);
		//showNotifyInChatWindow(AStreamId, tr("Registration on SIP server has failed."), MNI_SIPPHONE_CALL_HANGUP);
	}
}

void SipPhone::closeStream(const QString &AStreamId)
{
	if (FStanzaProcessor && FStreams.contains(AStreamId))
	{
		ISipStream &stream = FStreams[AStreamId];
		bool isResult = FPendingRequests.contains(AStreamId);
		if (stream.state!=ISipStream::SS_CLOSE || isResult)
		{
			LogDetail(QString("[SipPhone] Closing SIP stream, sid='%1'").arg(AStreamId));


			Stanza close("iq");
			QDomElement closeElem;
			if (isResult)
			{
				close.setType("result").setId(FPendingRequests.value(AStreamId)).setTo(stream.contactJid.eFull());
				closeElem = close.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(close.createElement("closed")).toElement();
			}
			else
			{
				close.setType("set").setId(FStanzaProcessor->newId()).setTo(stream.contactJid.eFull());
				closeElem = close.addElement("query",NS_RAMBLER_SIP_PHONE).appendChild(close.createElement("close")).toElement();
			}
			closeElem.setAttribute("sid",stream.sid);

			if(stream.errFlag == ISipStream::EF_REGFAIL)
				closeElem.setAttribute("reason", tr("Registration on SIP server has failed."));
			else if(stream.errFlag == ISipStream::EF_DEVERR)
				closeElem.setAttribute("reason", stream.failReason);

			stream.state = ISipStream::SS_CLOSE;
			emit streamStateChanged(AStreamId,stream.state);

			if (isResult ? FStanzaProcessor->sendStanzaOut(stream.streamJid,close) : FStanzaProcessor->sendStanzaRequest(this,stream.streamJid,close,CLOSE_TIMEOUT))
			{
				LogDetail(QString("[SipPhone] Close request sent to '%1', sid='%2', id='%3'").arg(stream.contactJid.full()).arg(stream.sid).arg(close.id()));
				if (!isResult)
					FCloseRequests.insert(close.id(),AStreamId);
				else
					removeStream(AStreamId);
			}
			else
			{
				//Не удалось отправить запрос, возможно связь с сервером прервалась, считаем сессию закрытой
				LogError(QString("[SipPhone] Failed to send close request to '%1', sid='%2'").arg(stream.contactJid.full()).arg(stream.sid));
				removeStream(AStreamId);
			}
			FPendingRequests.remove(AStreamId);
			removeIncomingNotify(AStreamId);
		}
	}
}


IMetaTabWindow *SipPhone::findMetaWindow(const QString &AMetaId)
{
	if (FMetaContacts)
	{
		foreach(IMetaTabWindow *window, FMetaContacts->metaTabWindows())
			if (window->metaId() == AMetaId)
				return window;
	}
	return NULL;
}

QString SipPhone::findMetaId(const Jid& AStreamJid, const Jid& AContactJid) const
{
	IMetaRoster* mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(AStreamJid) : NULL;
	return mroster!=NULL ? mroster->itemMetaContact(AContactJid) : QString::null;
}

RCallControl *SipPhone::newRCallControl(const QString &AStreamId, RCallControl::CallSide ASide, IMetaTabWindow *AMetaWindow)
{
	RCallControl* pCallControl = new RCallControl(AStreamId, ASide, AMetaWindow->instance());
	pCallControl->setStreamJid(AMetaWindow->metaRoster()->streamJid());
	pCallControl->setMetaId(AMetaWindow->metaId());

	connect(pCallControl, SIGNAL(acceptCall()), SLOT(onAcceptCall()));
	connect(pCallControl, SIGNAL(redialCall()), SLOT(onRedialCall()));
	connect(pCallControl, SIGNAL(abortCall()),  SLOT(onAbortCall()));
	connect(pCallControl, SIGNAL(callbackCall()), SLOT(onCallbackCall()));
	connect(pCallControl, SIGNAL(hangupCall()), FSipPhone, SLOT(hangup()));

	// Обработка: при закрытии окна управления звонком, нужно вернуть кнопку вызова в исходное состояние
	connect(pCallControl, SIGNAL(closeAndDelete(bool)), this, SLOT(onCloseCallControl(bool)));

	connect(pCallControl, SIGNAL(startCamera()), FSipPhone, SIGNAL(proxyStartCamera()));
	connect(pCallControl, SIGNAL(stopCamera()), FSipPhone, SIGNAL(proxyStopCamera()));
	//connect(pCallControl, SIGNAL(camResolutionChange(bool)), FSipPhone, SIGNAL(proxyCamResolutionChange(bool)));
	connect(pCallControl, SIGNAL(micStateChange(bool)), FSipPhone, SIGNAL(proxySuspendStateChange(bool)));

	connect(FSipPhone, SIGNAL(camPresentChanged(bool)), pCallControl, SLOT(setCameraEnabled(bool)));
	connect(FSipPhone, SIGNAL(micPresentChanged(bool)), pCallControl, SLOT(setMicEnabled(bool)));
	connect(FSipPhone, SIGNAL(volumePresentChanged(bool)), pCallControl, SLOT(setVolumeEnabled(bool)));

	FCallActions[AMetaWindow->metaId()]->setChecked(true);
	if(FBackupCallActionMenu == NULL)
	{
		FBackupCallActionMenu = FCallActions[AMetaWindow->metaId()]->menu();
		FCallActions[AMetaWindow->metaId()]->setMenu(NULL);
	}

	AMetaWindow->insertTopWidget(0, pCallControl);
	FCallControls.insert(AMetaWindow->metaId(), pCallControl);

	return pCallControl;
}

void SipPhone::showCallControlTab(const QString& sid)
{
	if(!FStreams.contains(sid))
		return;

	ISipStream &stream = FStreams[sid];
	QString metaId = findMetaId(stream.streamJid, stream.contactJid);
	if(!FCallControls.contains(metaId))
	{
		IMetaTabWindow* metaWindow = FMetaContacts->findMetaTabWindow(stream.streamJid, metaId);
		if(metaWindow != NULL)
		{
			newRCallControl(sid,RCallControl::Receiver,metaWindow);
		}
	}
	else
	{
		RCallControl *pCallControl = FCallControls.value(metaId);
		if(pCallControl)
		{
			if(pCallControl->side() == RCallControl::Caller)
			{
				pCallControl->callSideChange(RCallControl::Receiver);
			}
			pCallControl->setSessionId(sid);
			pCallControl->callStatusChange(RCallControl::Ringing);
		}
	}
}

void SipPhone::insertIncomingNotify(const ISipStream &AStream)
{
	INotification notify;
	notify.kinds = FNotifications ? FNotifications->enabledTypeNotificationKinds(NNT_SIPPHONE_CALL) : 0;
	if (notify.kinds > 0)
	{
		QString message = tr("Calling you...");
		QString name = FNotifications->contactName(AStream.streamJid,AStream.contactJid);

		notify.typeId = NNT_SIPPHONE_CALL;
		notify.data.insert(NDR_STREAM_JID,AStream.streamJid.full());
		notify.data.insert(NDR_CONTACT_JID,AStream.contactJid.full());
		notify.data.insert(NDR_ICON_KEY,MNI_SIPPHONE_CALL);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_ORDER,RNO_SIPPHONE_CALL);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
		notify.data.insert(NDR_ROSTER_HOOK_CLICK,false);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
		notify.data.insert(NDR_ROSTER_FOOTER,message);
		notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::green));
		notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(message));

		if (FMessageProcessor)
			FMessageProcessor->createMessageWindow(AStream.streamJid,AStream.contactJid,Message::Chat,IMessageHandler::SM_ASSIGN);

		IChatWindow *winow = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStream.streamJid,AStream.contactJid) : NULL;
		if (winow)
		{
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SIP_CALL);
			notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,0);
			notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
			notify.data.insert(NDR_TABPAGE_TOOLTIP,message);
			notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_SIPPHONE_TABBARITEM_CALL);
		}

		SipCallNotifyer *callNotifyer = new SipCallNotifyer(name, tr("Incoming call"), IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL), FNotifications->contactAvatar(AStream.streamJid,AStream.contactJid));
		callNotifyer->setProperty("streamId", AStream.sid);
		connect(callNotifyer, SIGNAL(accepted()), SLOT(onAcceptStreamByAction()));
		connect(callNotifyer, SIGNAL(rejected()), SLOT(onCloseStreamByAction()));
		connect(this, SIGNAL(hideCallNotifyer()), callNotifyer, SLOT(disappear()));
		callNotifyer->appear();

		Action *acceptCall = new Action(this);
		acceptCall->setText(tr("Accept"));
		acceptCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(acceptCall,SIGNAL(triggered()),SLOT(onAcceptStreamByAction()));
		notify.actions.append(acceptCall);

		Action *declineCall = new Action(this);
		declineCall->setText(tr("Decline"));
		declineCall->setData(ADR_STREAM_ID,AStream.sid);
		connect(declineCall,SIGNAL(triggered()),SLOT(onCloseStreamByAction()));
		notify.actions.append(declineCall);

		FIncomingNotifies.insert(FNotifications->appendNotification(notify), AStream.sid);
	}
}

void SipPhone::removeIncomingNotify(const QString &AStreamId)
{
	if (FNotifications)
		FNotifications->removeNotification(FIncomingNotifies.key(AStreamId));
	emit hideCallNotifyer();
}

void SipPhone::insertMissedNotify(const ISipStream &AStream)
{
	INotification notify;
	notify.kinds = FNotifications ? FNotifications->enabledTypeNotificationKinds(NNT_SIPPHONE_MISSEDCALL) : 0;
	if (notify.kinds > 0)
	{
		int callsCount = 1;
		IChatWindow *winow = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStream.streamJid,AStream.contactJid) : NULL;
		if (winow)
		{
			int oldNotifyId = FMissedNotifies.key(winow);
			if (oldNotifyId > 0)
			{
				callsCount += FNotifications->notificationById(oldNotifyId).data.value(NDR_TABPAGE_NOTIFYCOUNT).toInt();
				removeMissedNotify(winow);
			}
		}
		QString message = tr("%n missed call(s)","",callsCount);

		notify.typeId = NNT_SIPPHONE_MISSEDCALL;
		notify.data.insert(NDR_STREAM_JID,AStream.streamJid.full());
		notify.data.insert(NDR_CONTACT_JID,AStream.contactJid.full());
		notify.data.insert(NDR_ICON_KEY,MNI_SIPPHONE_CALL_MISSED);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_ORDER,RNO_SIPPHONE_MISSED_CALL);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
		notify.data.insert(NDR_ROSTER_HOOK_CLICK,true);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,false);
		notify.data.insert(NDR_ROSTER_FOOTER,message);
		notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::yellow));

		if (winow)
		{
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)winow->instance());
			notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SIP_MISSED_CALL);
			notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,callsCount);
			notify.data.insert(NDR_TABPAGE_ICONBLINK,false);
			notify.data.insert(NDR_TABPAGE_TOOLTIP,message);
			notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_SIPPHONE_TABBARITEM_CALL);

			connect(winow->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
			connect(winow->instance(),SIGNAL(tabPageDestroyed()),SLOT(onChatWindowDestroyed()));
		}

		FMissedNotifies.insert(FNotifications->appendNotification(notify), winow);
	}
}

void SipPhone::removeMissedNotify(IChatWindow *AWindow)
{
	if (FNotifications)
		FNotifications->removeNotification(FMissedNotifies.key(AWindow));
}

void SipPhone::showNotifyInChatWindow(const QString &AStreamId, const QString &ANotify, const QString &AIconId)
{
	if (FMessageProcessor && FMessageWidgets && FStreams.contains(AStreamId))
	{
		ISipStream &stream = FStreams[AStreamId];
		if (FMessageProcessor->createMessageWindow(stream.streamJid,stream.contactJid,Message::Chat,IMessageHandler::SM_ASSIGN))
		{
			IChatWindow *window = FMessageWidgets->findChatWindow(stream.streamJid,stream.contactJid);
			if (window)
			{
				IMessageContentOptions options;
				options.kind = IMessageContentOptions::Status;
				options.type |= IMessageContentOptions::Notification;
				options.direction = IMessageContentOptions::DirectionIn;
				if (!stream.contentId.isNull())
				{
					options.action = IMessageContentOptions::Replace;
					options.contentId = stream.contentId;
				}
				options.time = QDateTime::currentDateTime();
				options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;

				QUrl iconFile = QUrl::fromLocalFile(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(AIconId));
				QString html = QString("<img src='%1'/> <b>%2</b>").arg(iconFile.toString()).arg(Qt::escape(ANotify));

				stream.contentId = window->viewWidget()->changeContentHtml(html,options);
			}
		}
	}
}

void SipPhone::removeStream(const QString &AStreamId)
{
	if (FStreams.contains(AStreamId))
	{
		ISipStream &stream = FStreams[AStreamId];
		stream.state = ISipStream::SS_CLOSED;
		emit streamStateChanged(AStreamId, stream.state);
		emit streamRemoved(AStreamId);
		FStreams.remove(AStreamId);
	}
}

void SipPhone::onOpenStreamByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		openStream(streamJid, contactJid);
	}
}

void SipPhone::onAcceptStreamByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		acceptStream(streamId);
	}
	else
	{
		SipCallNotifyer *callNotifyer = qobject_cast<SipCallNotifyer*>(sender());
		if (callNotifyer)
		{
			QString streamId = callNotifyer->property("streamId").toString();
			acceptStream(streamId);
		}
	}
}

void SipPhone::onCloseStreamByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamId = action->data(ADR_STREAM_ID).toString();
		if (FStreams.contains(streamId))
			FStreams[streamId].timeout = false;
		closeStream(streamId);
	}
	else
	{
		SipCallNotifyer * callNotifyer = qobject_cast<SipCallNotifyer*>(sender());
		if (callNotifyer)
		{
			QString streamId = callNotifyer->property("streamId").toString();
			if (FStreams.contains(streamId))
				FStreams[streamId].timeout = false;
			closeStream(streamId);
		}
	}
}

void SipPhone::onNotificationActivated(int ANotifyId)
{
	if (FIncomingNotifies.contains(ANotifyId))
	{
		acceptStream(FIncomingNotifies.value(ANotifyId));
		FNotifications->removeNotification(ANotifyId);
	}
	else if (FMissedNotifies.contains(ANotifyId))
	{
		FMissedNotifies.value(ANotifyId)->showTabPage();
		FNotifications->removeNotification(ANotifyId);
	}
}

void SipPhone::onNotificationRemoved(int ANotifyId)
{
	FIncomingNotifies.remove(ANotifyId);
	FMissedNotifies.remove(ANotifyId);
}

void SipPhone::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ASelected);
	Q_UNUSED(AMenu);

	//////////// В случае обычных контактов
	//////////if (AIndex->type()==RIT_CONTACT && ASelected.count() < 2)
	//////////{
	//////////	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	//////////	Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
	//////////	if (isSupported(streamJid,contactJid))
	//////////	{
	//////////		if (findStream(streamJid,contactJid).isEmpty())
	//////////		{
	//////////			Action *action = new Action(AMenu);
	//////////			action->setText(tr("Call"));
	//////////			action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
	//////////			action->setData(ADR_STREAM_JID,streamJid.full());
	//////////			action->setData(ADR_CONTACT_JID,contactJid.full());
	//////////			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
	//////////			AMenu->addAction(action,AG_RVCM_SIPPHONE_CALL,true);
	//////////		}
	//////////	}
	//////////	return;
	//////////}

	//////////// В случае метаконтактов
	//////////if ( AIndex->type()==RIT_METACONTACT && ASelected.count() < 2)
	//////////{
	//////////	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	//////////	QString metaId = AIndex->data(RDR_META_ID).toString();

	//////////	IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(streamJid);
	//////////	IPresence *presence = FPresencePlugin ? FPresencePlugin->findPresence(streamJid) : NULL;

	//////////	if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
	//////////	{
	//////////		IMetaContact metaContact = metaRoster->metaContact(metaId);
	//////////		if(metaContact.items.size() > 0)
	//////////		{
	//////////			foreach(Jid contactJid, metaContact.items)
	//////////			{
	//////////				QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

	//////////				if(pItems.size() > 0)
	//////////				{
	//////////					foreach(IPresenceItem pItem, pItems)
	//////////					{
	//////////						Jid contactJidWithPresence = pItem.itemJid;
	//////////						if(isSupported(streamJid, contactJidWithPresence))
	//////////						{
	//////////							if (findStream(streamJid, contactJidWithPresence).isEmpty())
	//////////							{
	//////////								Action *action = new Action(AMenu);
	//////////								action->setText(tr("Call"));
	//////////								action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
	//////////								action->setData(ADR_STREAM_JID,streamJid.full());
	//////////								action->setData(ADR_CONTACT_JID,contactJidWithPresence.full());
	//////////								action->setData(ADR_METAID_WINDOW, metaId);
	//////////								connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
	//////////								AMenu->addAction(action,AG_RVCM_SIPPHONE_CALL,true);
	//////////							}
	//////////						}
	//////////					}
	//////////				}
	//////////			}
	//////////		}
	//////////	}
	//////////}
}

void SipPhone::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ALabelId);
	Q_UNUSED(AToolTips);
	Q_UNUSED(AToolBarChanger);
	return;
	//////////Q_UNUSED(AToolTips);
	//////////// В случае обычных контактов
	//////////if (ALabelId==RLID_DISPLAY && AIndex->type()==RIT_CONTACT)
	//////////{
	//////////	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	//////////	Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
	//////////	if (isSupported(streamJid, contactJid))
	//////////	{
	//////////		if (findStream(streamJid, contactJid).isEmpty())
	//////////		{
	//////////			Action *action = new Action(AToolBarChanger->toolBar());
	//////////			action->setText(tr("Call"));
	//////////			action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
	//////////			action->setData(ADR_STREAM_JID,streamJid.full());
	//////////			action->setData(ADR_CONTACT_JID,contactJid.full());
	//////////			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
	//////////			AToolBarChanger->insertAction(action);
	//////////		}
	//////////	}
	//////////	return;
	//////////}

	//////////// В случае метаконтактов
	//////////if (ALabelId==RLID_DISPLAY && AIndex->type()==RIT_METACONTACT)
	//////////{
	//////////	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	//////////	QString metaId = AIndex->data(RDR_META_ID).toString();

	//////////	IMetaRoster* metaRoster = FMetaContacts->findMetaRoster(streamJid);
	//////////	IPresence *presence = FPresencePlugin ? FPresencePlugin->findPresence(streamJid) : NULL;

	//////////	if(metaRoster != NULL && metaRoster->isEnabled() && presence && presence->isOpen())
	//////////	{
	//////////		IMetaContact metaContact = metaRoster->metaContact(metaId);
	//////////		if(metaContact.items.size() > 0)
	//////////		{
	//////////			foreach(Jid contactJid, metaContact.items)
	//////////			{
	//////////				QList<IPresenceItem> pItems = presence->presenceItems(contactJid);

	//////////				if(pItems.size() > 0)
	//////////				{
	//////////					foreach(IPresenceItem pItem, pItems)
	//////////					{
	//////////						Jid contactJidWithPresence = pItem.itemJid;
	//////////						if(isSupported(streamJid, contactJidWithPresence))
	//////////						{
	//////////							if (findStream(streamJid, contactJidWithPresence).isEmpty())
	//////////							{
	//////////								Action *action = new Action(AToolBarChanger->toolBar());
	//////////								action->setText(tr("Call"));
	//////////								action->setIcon(RSR_STORAGE_MENUICONS,MNI_SIPPHONE_CALL);
	//////////								action->setData(ADR_STREAM_JID,streamJid.full());
	//////////								action->setData(ADR_CONTACT_JID,contactJidWithPresence.full());
	//////////								//action->setData(ADR_CONTACT_JID,contactJid.full());
	//////////								action->setData(ADR_METAID_WINDOW, metaId);
	//////////								connect(action,SIGNAL(triggered(bool)),SLOT(onOpenStreamByAction(bool)));
	//////////								AToolBarChanger->insertAction(action);
	//////////							}
	//////////						}
	//////////					}
	//////////				}
	//////////			}
	//////////		}
	//////////	}
	//////////}

}

Jid SipPhone::getContactWithPresence(const Jid &AStreamJid, const QString &AMetaId) const
{
	IMetaRoster* mroster = FMetaContacts->findMetaRoster(AStreamJid);
	if(mroster!=NULL && mroster->isEnabled())
	{
		IMetaContact metaContact = mroster->metaContact(AMetaId);
		foreach(Jid contactJid, metaContact.items)
		{
			foreach(IPresenceItem pItem, mroster->itemPresences(contactJid))
			{
				if(isSupported(AStreamJid, pItem.itemJid))
					return pItem.itemJid;
			}
		}
	}
	return Jid::null;
}

Q_EXPORT_PLUGIN2(plg_sipphone, SipPhone)
