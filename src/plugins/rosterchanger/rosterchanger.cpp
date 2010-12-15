#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_FROM_STREAM_JID Action::DR_Parametr2
#define ADR_SUBSCRIPTION    Action::DR_Parametr2
#define ADR_NICK            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_TO_GROUP        Action::DR_Parametr4
#define ADR_CHATNOTICE_ID   Action::DR_UserDefined+1
#define ADR_NOTIFY_ID       Action::DR_UserDefined+2
#define ADR_NOTICE_ACTION   Action::DR_UserDefined+3

enum NoticeActions
{
	NTA_NO_ACTIONS          = 0x00,
	NTA_ADD_CONTACT         = 0x01,
	NTA_ASK_SUBSCRIBE       = 0x02,
	NTA_SUBSCRIBE           = 0x04,
	NTA_UNSUBSCRIBE         = 0x08,
	NTA_CLOSE               = 0x10
};

enum NotifyActions
{
	NFA_NO_ACTIONS          = 0x00,
	NFA_SUBSCRIBE           = 0x01,
	NFA_UNSUBSCRIBE         = 0x02,
	NFA_CLOSE               = 0x04
};

void GroupMenu::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QAction *action = actionAt(AEvent->pos());
	if (action)
		action->trigger();
	else
		Menu::mouseReleaseEvent(AEvent);
}

RosterChanger::RosterChanger()
{
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FRostersModel = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FXmppUriQueries = NULL;
	FMultiUserChatPlugin = NULL;
	FAccountManager = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
}

RosterChanger::~RosterChanger()
{

}

//IPlugin
void RosterChanger::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Editor");
	APluginInfo->description = tr("Allows to edit roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(ROSTER_UUID);
}

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscriptionSent(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionSent(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscriptionReceived(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionReceived(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)), SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
		if (FMultiUserChatPlugin)
		{
			connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowDestroyed(IChatWindow *)),SLOT(onChatWindowDestroyed(IChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
	if (FNotifications)
	{
		uchar kindMask = INotification::RosterIcon|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound|INotification::AutoActivate;
		uchar kindDefs = INotification::RosterIcon|INotification::TabPage|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound;
		FNotifications->insertNotificator(NID_SUBSCRIPTION,OWO_NOTIFICATIONS_SUBSCRIPTIONS,QString::null,kindMask,kindDefs);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
	}
	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	if (FMainWindowPlugin)
	{
		Menu *mmenu = FMainWindowPlugin->mainWindow()->mainMenu();

		Action *action = new Action(mmenu);
		action->setText(tr("Add group"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_GROUP);
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddGroupDialog(bool)));
		mmenu->addAction(action,AG_MMENU_RCHAGER_ADD_GROUP);

		action = new Action(mmenu);
		action->setText(tr("Add contact"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_CONTACT);
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddContactDialog(bool)));
		mmenu->addAction(action,AG_MMENU_RCHAGER_ADD_CONTACT);

		action = new Action(mmenu);
		action->setText(tr("Add account"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_ACCOUNT);
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddAccountDialog(bool)));
		mmenu->addAction(action,AG_MMENU_RCHAGER_ADD_ACCOUNT);
	}
	qsrand(QDateTime::currentDateTime().toTime_t());
	return true;
}

bool RosterChanger::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_AUTOSUBSCRIBE, false);
	Options::setDefaultValue(OPV_ROSTER_AUTOUNSUBSCRIBE, true);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> RosterChanger::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	Q_UNUSED(ANodeId); Q_UNUSED(AParent);
	QMultiMap<int, IOptionsWidget *> widgets;
	//if (FOptionsManager && ANode == OPN_ROSTER)
	//{
	//	AOrder = OWO_ROSTER_CHANGER;

	//	IOptionsContainer *container = FOptionsManager->optionsContainer(AParent);
	//	container->appendChild(Options::node(OPV_ROSTER_AUTOSUBSCRIBE),tr("Auto accept subscription requests"));
	//	container->appendChild(Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE),tr("Auto unsubscribe contacts"));
	//	return container;
	//}
	return widgets;
}

int RosterChanger::rosterDataOrder() const
{
	return RDHO_ROSTER_AUTH;
}

QList<int> RosterChanger::rosterDataRoles() const
{
	static QList<int> dataRoles = QList<int>() 
		<< RDR_FOOTER_TEXT << Qt::DecorationRole; 
	return dataRoles;
}

QList<int> RosterChanger::rosterDataTypes() const
{
	static QList<int> dataTypes = QList<int>() << RIT_CONTACT;
	return dataTypes;
}

QVariant RosterChanger::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	if (AIndex->type() == RIT_CONTACT)
	{
		static bool block = false;
		if (!block)
		{
			block = true;
			Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
			Jid contactJid = AIndex->data(RDR_BARE_JID).toString();
			QString ask = AIndex->data(RDR_ASK).toString();
			QString subs = AIndex->data(RDR_SUBSCRIBTION).toString();
			if (FSubscriptionRequests.contains(streamJid,contactJid))
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Requests authorization"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCR_REQUEST);
				}
			}
			else if (ask == SUBSCRIPTION_SUBSCRIBE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Sent an authorization request"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCR_NONE);
				}
			}
			else if (subs == SUBSCRIPTION_NONE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Not authorized"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCR_NONE);
				}
			}
			block = false;
		}
	}
	return data;
}

bool RosterChanger::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

//IRostersDragDropHandler
Qt::DropActions RosterChanger::rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent);
	Q_UNUSED(ADrag);
	int indexType = AIndex.data(RDR_TYPE).toInt();
	if (indexType==RIT_CONTACT || indexType==RIT_GROUP)
		return Qt::CopyAction|Qt::MoveAction;
	return Qt::IgnoreAction;
}

bool RosterChanger::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		//operator>>(stream, indexData);
		stream >> indexData;

		int indexType = indexData.value(RDR_TYPE).toInt();
		if (indexType==RIT_CONTACT || (indexType==RIT_GROUP && AEvent->source()==FRostersView->instance()))
			return true;
	}
	return false;
}

bool RosterChanger::rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover)
{
	Q_UNUSED(AEvent);
	int indexType = AHover.data(RDR_TYPE).toInt();
//	if (indexType==RIT_GROUP || indexType==RIT_STREAM_ROOT)
//	{
//		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AHover.data(RDR_STREAM_JID).toString()) : NULL;
//		if (roster && roster->isOpen())
//			return true;
//	}
	if ((indexType == RIT_GROUP) || (AHover.parent().isValid() && AHover.parent().data(RDR_TYPE).toInt() == RIT_GROUP))
	{
		return true;
	}

	return false;
}

void RosterChanger::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RosterChanger::rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu)
{
	int hoverType = AIndex.data(RDR_TYPE).toInt();
	if (AEvent->dropAction()!=Qt::IgnoreAction && (hoverType==RIT_GROUP || hoverType==RIT_STREAM_ROOT || hoverType == RIT_CONTACT))
	{
		Jid hoverStreamJid = AIndex.data(RDR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(hoverStreamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			//operator>>(stream, indexData);
			stream >> indexData;

			int indexType = indexData.value(RDR_TYPE).toInt();
			Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
			bool isNewContact = indexType==RIT_CONTACT && !roster->rosterItem(indexData.value(RDR_BARE_JID).toString()).isValid;

			if (!isNewContact && (hoverStreamJid && indexStreamJid))
			{
				Action *copyAction = new Action(AMenu);
				copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
				copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				copyAction->setData(ADR_TO_GROUP,hoverType==RIT_GROUP ? AIndex.data(RDR_GROUP) : AIndex.parent().data(RDR_GROUP));

				Action *moveAction = new Action(AMenu);
				moveAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
				moveAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				moveAction->setData(ADR_TO_GROUP,hoverType==RIT_GROUP ? AIndex.data(RDR_GROUP) : AIndex.parent().data(RDR_GROUP));

				if (indexType == RIT_CONTACT)
				{
					copyAction->setText(tr("Copy contact"));
					copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyItemToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);

					moveAction->setText(tr("Move contact"));
					moveAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
					moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveItemToGroup(bool)));
					AMenu->addAction(moveAction,AG_DEFAULT,true);
				}
				else
				{
					copyAction->setText(tr("Copy group"));
					copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyGroupToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);

					moveAction->setText(tr("Move group"));
					moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveGroupToGroup(bool)));
					AMenu->addAction(moveAction,AG_DEFAULT,true);
				}
				AMenu->setDefaultAction(AEvent->dropAction()==Qt::MoveAction ? moveAction : copyAction);
				return true;
			}
			else
			{
				Action *copyAction = new Action(AMenu);
				copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				copyAction->setData(ADR_TO_GROUP,hoverType==RIT_GROUP ? AIndex.data(RDR_GROUP) : QVariant(QString("")));

				if (indexType == RIT_CONTACT)
				{
					copyAction->setText(isNewContact ? tr("Add contact") : tr("Copy contact"));
					copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
					copyAction->setData(ADR_NICK,indexData.value(RDR_NAME));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onAddItemToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);
				}
				else
				{
					copyAction->setText(tr("Copy group"));
					copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					copyAction->setData(ADR_FROM_STREAM_JID,indexStreamJid.full());
					copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onAddGroupToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);
				}
				AMenu->setDefaultAction(copyAction);
				return true;
			}
		}
	}
	return false;
}

bool RosterChanger::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "roster")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && !roster->rosterItem(AContactJid).isValid)
		{
			IAddContactDialog *dialog = showAddContactDialog(AStreamJid);
			if (dialog)
			{
				dialog->setContactJid(AContactJid);
				dialog->setNickName(AParams.contains("name") ? AParams.value("name") : AContactJid.node());
				dialog->setGroup(AParams.contains("group") ? AParams.value("group") : QString::null);
				dialog->instance()->show();
			}
		}
		return true;
	}
	else if (AAction == "remove")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && roster->rosterItem(AContactJid).isValid)
		{
			if (QMessageBox::question(NULL, tr("Remove contact"),
						  tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(AContactJid.hBare()),
						  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItem(AContactJid);
			}
		}
		return true;
	}
	else if (AAction == "subscribe")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
		{
			if (QMessageBox::question(NULL, tr("Subscribe for contact presence"),
						  tr("You are assured that wish to subscribe for a contact <b>%1</b> presence?").arg(AContactJid.hBare()),
						  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->sendSubscription(AContactJid, IRoster::Subscribe);
			}
		}
		return true;
	}
	else if (AAction == "unsubscribe")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_NONE && ritem.subscription!=SUBSCRIPTION_FROM)
		{
			if (QMessageBox::question(NULL, tr("Unsubscribe from contact presence"),
						  tr("You are assured that wish to unsubscribe from a contact <b>%1</b> presence?").arg(AContactJid.hBare()),
						  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->sendSubscription(AContactJid, IRoster::Unsubscribe);
			}
		}
		return true;
	}
	return false;
}

//IRosterChanger
bool RosterChanger::isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (Options::node(OPV_ROSTER_AUTOSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoSubscribe;
	return false;
}

bool RosterChanger::isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoUnsubscribe;
	return false;
}

bool RosterChanger::isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).silent;
	return false;
}

void RosterChanger::insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr)
{
	AutoSubscription &asubscr = FAutoSubscriptions[AStreamJid][AContactJid.bare()];
	asubscr.silent = ASilently;
	asubscr.autoSubscribe = ASubscr;
	asubscr.autoUnsubscribe = AUnsubscr;
}

void RosterChanger::removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid)
{
	FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		IRosterItem ritem = roster->rosterItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,true,false);
	}
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		IRosterItem ritem = roster->rosterItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Unsubscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_NONE)
			roster->sendSubscription(AContactJid,IRoster::Unsubscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,false,true);
	}
}

IAddContactDialog *RosterChanger::showAddContactDialog(const Jid &AStreamJid)
{
	IRoster *roster = FRosterPlugin ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		AddContactDialog *dialog = new AddContactDialog(this,FPluginManager,AStreamJid);
		connect(roster->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
		emit addContactDialogCreated(dialog);
		dialog->show();
		return dialog;
	}
	return NULL;
}

QString RosterChanger::subscriptionNotify(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	IRosterItem ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
	QString name = ritem.isValid && !ritem.name.isEmpty() ? ritem.name : AContactJid.bare();

	switch (ASubsType)
	{
	case IRoster::Subscribe:
		return tr("%1 requests authorization (permission to see your status and mood).").arg(name);
	case IRoster::Subscribed:
		return tr("%1 authorized you to see its status and mood.").arg(name);
	case IRoster::Unsubscribe:
		return tr("%1 refused authorization to see your status and mood.").arg(name);
	case IRoster::Unsubscribed:
		return tr("%1 removed your authorization to view its status and mood.").arg(name);
	}

	return QString::null;
}

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent)
{
	Menu *menu = new Menu(AParent);
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AData.value(ADR_STREAM_JID).toString()) : NULL;
	if (roster)
	{
		QString group;
		QString groupDelim = roster->groupDelimiter();
		QHash<QString,Menu *> menus;
		QSet<QString> allGroups = roster->groups() + FEmptyGroups.toSet();
		foreach(group,allGroups)
		{
			Menu *parentMenu = menu;
			QList<QString> groupTree = group.split(groupDelim,QString::SkipEmptyParts);
			QString groupName;
			int index = 0;
			while (index < groupTree.count())
			{
				if (groupName.isEmpty())
					groupName = groupTree.at(index);
				else
					groupName += groupDelim + groupTree.at(index);

				if (!menus.contains(groupName))
				{
					Menu *groupMenu = new Menu(parentMenu);
					groupMenu->setTitle(groupTree.at(index));
					groupMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);

					if (!AExceptGroups.contains(groupName))
					{
						Action *curGroupAction = new Action(groupMenu);
						curGroupAction->setText(tr("This group"));
						curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_THIS_GROUP);
						curGroupAction->setData(AData);
						curGroupAction->setData(ADR_TO_GROUP,groupName);
						connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
						groupMenu->addAction(curGroupAction,AG_RVCM_ROSTERCHANGER_GROUP+1);
					}

					if (ANewGroup)
					{
						Action *newGroupAction = new Action(groupMenu);
						newGroupAction->setText(tr("Create new..."));
						newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
						newGroupAction->setData(AData);
						newGroupAction->setData(ADR_TO_GROUP,groupName+groupDelim);
						connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
						groupMenu->addAction(newGroupAction,AG_RVCM_ROSTERCHANGER_GROUP+1);
					}

					menus.insert(groupName,groupMenu);
					parentMenu->addAction(groupMenu->menuAction(),AG_RVCM_ROSTERCHANGER_GROUP,true);
					parentMenu = groupMenu;
				}
				else
					parentMenu = menus.value(groupName);

				index++;
			}
		}

		if (ARootGroup)
		{
			Action *curGroupAction = new Action(menu);
			curGroupAction->setText(tr("Root"));
			curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ROOT_GROUP);
			curGroupAction->setData(AData);
			curGroupAction->setData(ADR_TO_GROUP,"");
			connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(curGroupAction,AG_RVCM_ROSTERCHANGER_GROUP+1);
		}

		if (ANewGroup)
		{
			Action *newGroupAction = new Action(menu);
			newGroupAction->setText(tr("Create new..."));
			newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
			newGroupAction->setData(AData);
			newGroupAction->setData(ADR_TO_GROUP,groupDelim);
			connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(newGroupAction,AG_RVCM_ROSTERCHANGER_GROUP+1);
		}
	}
	return menu;
}

SubscriptionDialog *RosterChanger::createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		SubscriptionDialog *dialog = new SubscriptionDialog(this,FPluginManager,AStreamJid,AContactJid,ANotify,AMessage);
		connect(roster->instance(),SIGNAL(closed()),dialog->instance(),SLOT(reject()));
		emit subscriptionDialogCreated(dialog);
		return dialog;
	}
	return NULL;
}

IChatWindow *RosterChanger::findChatNoticeWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IChatWindow *window, FChatNoticeWindow.values())
	{
		if (window->streamJid()==AStreamJid && (window->contactJid() && AContactJid))
			return window;
	}

	if (FMessageWidgets)
	{
		foreach(IChatWindow *window, FMessageWidgets->chatWindows())
		{
			if (window->streamJid()==AStreamJid && (window->contactJid() && AContactJid))
				return window;
		}
	}

	return NULL;
}

IChatNotice RosterChanger::createChatNotice(int APriority, int AActions, const QString &ANotify, const QString &AText) const
{
	IChatNotice notice;
	notice.priority = APriority;
	//notice.iconKey = MNI_RCHANGER_SUBSCRIBTION;
	//notice.iconStorage = RSR_STORAGE_MENUICONS;
	notice.message = !AText.isEmpty() ? Qt::escape(ANotify)+"<br>"+Qt::escape(AText) : Qt::escape(ANotify);

	if (AActions & NTA_ADD_CONTACT)
	{
		Action *addAction = new Action;
		addAction->setText(tr("Add Contact"));
		addAction->setData(ADR_NOTICE_ACTION,NTA_ADD_CONTACT);
		connect(addAction,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
		notice.actions.append(addAction);
	}
	if (AActions & NTA_ASK_SUBSCRIBE)
	{
		Action *askauthAction = new Action;
		askauthAction->setText(tr("Request authorization"));
		askauthAction->setData(ADR_NOTICE_ACTION,NTA_ASK_SUBSCRIBE);
		askauthAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		connect(askauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(askauthAction);
	}
	if (AActions & NTA_SUBSCRIBE)
	{
		Action *authAction = new Action;
		authAction->setText(tr("Authorize"));
		authAction->setData(ADR_NOTICE_ACTION,NTA_SUBSCRIBE);
		authAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		connect(authAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(authAction);
	}
	if (AActions & NTA_UNSUBSCRIBE)
	{
		Action *noauthAction = new Action;
		noauthAction->setText(tr("Don`t Authorize"));
		noauthAction->setData(ADR_NOTICE_ACTION,NTA_UNSUBSCRIBE);
		noauthAction->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
		connect(noauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(noauthAction);
	}
	if (AActions & NTA_CLOSE)
	{
		Action *closeAction = new Action;
		closeAction->setText(tr("Close"));
		closeAction->setData(ADR_NOTICE_ACTION,NTA_CLOSE);
		notice.actions.append(closeAction);
	}

	return notice;
}

int RosterChanger::insertChatNotice(IChatWindow *AWindow, const IChatNotice &ANotice)
{
	int chatNoticeId = -1;
	if (AWindow)
	{
		int actions = 0;
		chatNoticeId = AWindow->noticeWidget()->insertNotice(ANotice);
		foreach(Action *action, ANotice.actions)
		{
			actions |= action->data(ADR_NOTICE_ACTION).toInt();
			action->setData(ADR_STREAM_JID,AWindow->streamJid().full());
			action->setData(ADR_CONTACT_JID,AWindow->contactJid().bare());
			action->setData(ADR_CHATNOTICE_ID, chatNoticeId);
			connect(action,SIGNAL(triggered(bool)),SLOT(onChatNoticeActionTriggered(bool)));
		}
		FChatNoticeWindow.insert(chatNoticeId,AWindow);
		FChatNoticeActions.insert(chatNoticeId,actions);
	}
	return chatNoticeId;
}

void RosterChanger::removeObsoleteChatNotices(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent)
{
	foreach(IChatWindow *window, FChatNoticeWindow.values())
	{
		if (window->streamJid()==AStreamJid && (window->contactJid() && AContactJid))
		{
			int chatNoticeId = FChatNoticeWindow.key(window);
			int actions = FChatNoticeActions.value(chatNoticeId);

			bool obsolete = false;
			if (ASubsType == IRoster::Subscribe)
			{
				if (ASent)
					obsolete = (actions & (NTA_ADD_CONTACT|NTA_ASK_SUBSCRIBE))>0;
			}
			else if (ASubsType == IRoster::Subscribed)
			{
				if (ASent)
					obsolete = (actions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
				else
					obsolete = (actions & (NTA_ADD_CONTACT|NTA_ASK_SUBSCRIBE))>0;
			}
			else if (ASubsType == IRoster::Unsubscribe)
			{
				if (ASent)
					obsolete = (actions & NTA_ASK_SUBSCRIBE)>0;
				else
					obsolete = (actions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
			}
			else if (ASubsType == IRoster::Unsubscribed)
			{
				if (ASent)
					obsolete = (actions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
			}

			if (obsolete)
				window->noticeWidget()->removeNotice(chatNoticeId);
		}
	}

}

QList<int> RosterChanger::findNotifies(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QList<int> notifies;
	foreach(int notifyId, FNotifyChatNotice.keys())
	{
		INotification notify = FNotifications->notificationById(notifyId);
		if (AStreamJid==notify.data.value(NDR_STREAM_JID).toString() && (AContactJid && notify.data.value(NDR_CONTACT_JID).toString()))
			notifies.append(notifyId);
	}
	return notifies;
}

QList<Action *> RosterChanger::createNotifyActions(int AActions)
{
	QList<Action *> actions;
	if (AActions & NFA_SUBSCRIBE)
	{
		Action *authAction = new Action;
		authAction->setText(tr("Authorize"));
		authAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		connect(authAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		actions.append(authAction);
	}
	if (AActions & NFA_UNSUBSCRIBE)
	{
		Action *noauthAction = new Action;
		noauthAction->setText(tr("Don`t Authorize"));
		noauthAction->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
		connect(noauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		actions.append(noauthAction);
	}
	if (AActions & NFA_CLOSE)
	{
		Action *closeAction = new Action;
		closeAction->setText(tr("Close"));
		actions.append(closeAction);
	}
	return actions;
}

void RosterChanger::removeNotifies(IChatWindow *AWindow)
{
	foreach(int notifyId, findNotifies(AWindow->streamJid(),AWindow->contactJid()))
		FNotifications->removeNotification(notifyId);
}

void RosterChanger::removeObsoleteNotifies(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent)
{
	foreach(int notifyId, findNotifies(AStreamJid, AContactJid))
	{
		int subsType = FNotifications->notificationById(notifyId).data.value(NDR_SUBSCRIPTION_TYPE).toInt();

		bool remove = false;
		if (subsType == IRoster::Subscribe)
		{
			if (ASent)
				remove = ASubsType==IRoster::Subscribed || ASubsType==IRoster::Unsubscribed;
			else
				remove = ASubsType==IRoster::Unsubscribe;
		}
		else if (subsType == IRoster::Subscribed)
		{
			if (!ASent)
				remove = ASubsType==IRoster::Unsubscribed;
		}
		else if (subsType == IRoster::Unsubscribe)
		{
			if (!ASent)
				remove = ASubsType==IRoster::Subscribe;
		}
		else if (subsType == IRoster::Unsubscribed)
		{
			if (ASent)
				remove = ASubsType==IRoster::Subscribe;
			else
				remove = ASubsType==IRoster::Subscribed;
		}

		if (remove)
			FNotifications->removeNotification(notifyId);
	}
}

void RosterChanger::showNotifyInChatWindow(IChatWindow *AWindow, const QString &ANotify, const QString &AText) const
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Status;
	options.type |= IMessageContentOptions::Notification;
	options.direction = IMessageContentOptions::DirectionIn;
	options.time = QDateTime::currentDateTime();

	QString message = !AText.isEmpty() ? ANotify +" (" +AText+ ")" : ANotify;
	AWindow->viewWidget()->changeContentText(message,options);
}

void RosterChanger::onShowAddContactDialog(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IAccount *account = FAccountManager ? FAccountManager->accounts().first() : NULL;
	if (action && account && account->isActive())
	{
		IAddContactDialog *dialog = showAddContactDialog(account->xmppStream()->streamJid());
		if (dialog)
		{
			dialog->setContactJid(action->data(ADR_CONTACT_JID).toString());
			dialog->setNickName(action->data(ADR_NICK).toString());
			dialog->setGroup(action->data(ADR_GROUP).toString());
		}
	}
}

void RosterChanger::onShowAddGroupDialog(bool)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accounts().value(0) : NULL;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(account!=NULL ? account->xmppStream()->streamJid() : Jid::null) : NULL;
	if (FRostersModel && roster)
	{
		QString newGroupName = QInputDialog::getText(NULL, tr("Add group"), tr("Enter new group name:"));
		if (!newGroupName.isEmpty() && !newGroupName.contains(roster->groupDelimiter()) && FRostersModel->findGroup(newGroupName,roster->groupDelimiter(),RIT_GROUP,FRostersModel->streamRoot(roster->streamJid()))==NULL)
		{
			IRosterIndex *group = FRostersModel->createGroup(newGroupName,roster->groupDelimiter(),RIT_GROUP,FRostersModel->streamRoot(roster->streamJid()));
			if (group)
			{
				FEmptyGroups.append(newGroupName);
				group->setData(RDR_ALLWAYS_VISIBLE, group->data(RDR_ALLWAYS_VISIBLE).toInt()+1);
				connect(group->instance(),SIGNAL(childInserted(IRosterIndex *)),SLOT(onEmptyGroupChildInserted(IRosterIndex *)));
				connect(group->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onEmptyGroupIndexDestroyed(IRosterIndex *)));
			}
		}
	}
}

void RosterChanger::onShowAddAccountDialog(bool)
{
	if (FOptionsManager)
	{
		FOptionsManager->showOptionsDialog(OPN_GATEWAYS_ACCOUNTS);
	}
}

void RosterChanger::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IRoster *roster = FRosterPlugin ? FRosterPlugin->getRoster(streamJid) : NULL;
	if (roster && roster->isOpen())
	{
		int itemType = AIndex->data(RDR_TYPE).toInt();
		IRosterItem ritem = roster->rosterItem(AIndex->data(RDR_BARE_JID).toString());
		if (itemType == RIT_STREAM_ROOT)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Add contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			action->setData(ADR_STREAM_JID,AIndex->data(RDR_JID));
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_ADD_CONTACT,true);
		}
		else if (itemType == RIT_CONTACT || itemType == RIT_AGENT)
		{
			QString contactJid = AIndex->data(RDR_BARE_JID).toString();

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_CONTACT_JID,contactJid);

			if (FSubscriptionRequests.contains(streamJid,contactJid))
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Authorize"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_GRAND_AUTH);

				action = new Action(AMenu);
				action->setText(tr("Remove authorization"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_AUTH);
			}
			else if (ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO && ritem.ask.isEmpty())
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Request authorization"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_GRAND_AUTH);
			}

			Action *action = new Action(AMenu);
			action->setText(tr("Delete"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT);

			if (ritem.isValid)
			{
				data.insert(ADR_NICK,AIndex->data(RDR_NAME));
				data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

				action = new Action(AMenu);
				action->setText(tr("Rename..."));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
				action->setData(data);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);
				
				if (AIndex->type() == RIT_CONTACT)
				{
					GroupMenu *groupMenu = new GroupMenu(AMenu);
					groupMenu->setTitle(tr("Groups"));

					Action *blankGroupAction = new Action(groupMenu);
					blankGroupAction->setText(FRostersModel->blankGroupName());
					blankGroupAction->setData(data);
					blankGroupAction->setCheckable(true);
					blankGroupAction->setChecked(ritem.groups.isEmpty());
					connect(blankGroupAction,SIGNAL(triggered(bool)),SLOT(onChangeItemGroups(bool)));
					groupMenu->addAction(blankGroupAction,AG_DEFAULT-1,true);

					foreach (QString group, roster->groups())
					{
						Action *action = new Action(groupMenu);
						action->setText(group);
						action->setData(data);
						action->setData(ADR_TO_GROUP, group);
						action->setCheckable(true);
						action->setChecked(ritem.groups.contains(group));
						connect(action,SIGNAL(triggered(bool)),SLOT(onChangeItemGroups(bool)));
						groupMenu->addAction(action,AG_DEFAULT,true);
					}

					action = new Action(groupMenu);
					action->setText(tr("New group..."));
					action->setData(data);
					action->setData(ADR_TO_GROUP, roster->groupDelimiter());
					connect(action,SIGNAL(triggered(bool)),SLOT(onChangeItemGroups(bool)));
					groupMenu->addAction(action,AG_DEFAULT+1,true);

					AMenu->addAction(groupMenu->menuAction(),AG_RVCM_ROSTERCHANGER_GROUP);
				}
			}
			else
			{
				action = new Action(AMenu);
				action->setText(tr("Add contact"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				action->setData(ADR_STREAM_JID,streamJid);
				action->setData(ADR_CONTACT_JID,contactJid);
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_ADD_CONTACT);
			}

		}
		else if (itemType == RIT_GROUP)
		{
			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

			Action *action = new Action(AMenu);
			action->setText(tr("Rename group..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);
		}
	}
}

void RosterChanger::onContactSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
			int subsType = action->data(ADR_SUBSCRIPTION).toInt();
			if (subsType == IRoster::Subscribe)
				subscribeContact(streamJid,contactJid);
			else if (subsType == IRoster::Unsubscribe)
				unsubscribeContact(streamJid,contactJid);
		}
	}
}

void RosterChanger::onSendSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString rosterJid = action->data(ADR_CONTACT_JID).toString();
			int subsType = action->data(ADR_SUBSCRIPTION).toInt();
			roster->sendSubscription(rosterJid,subsType);
		}
	}
}

void RosterChanger::onSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Q_UNUSED(AText);
	if (ASubsType==IRoster::Subscribed || ASubsType==IRoster::Unsubscribed)
		FSubscriptionRequests.remove(ARoster->streamJid(),AItemJid);
	removeObsoleteChatNotices(ARoster->streamJid(),AItemJid,ASubsType,true);
	removeObsoleteNotifies(ARoster->streamJid(),AItemJid,ASubsType,true);
}

void RosterChanger::onSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText)
{
	INotification notify;
	IRosterItem ritem = ARoster->rosterItem(AItemJid);
	IChatWindow *chatWindow = findChatNoticeWindow(ARoster->streamJid(),AItemJid);
	QString name = FNotifications!=NULL ? FNotifications->contactName(ARoster->streamJid(),AItemJid) : AItemJid.node();
	QString notifyMessage = subscriptionNotify(ARoster->streamJid(),AItemJid,ASubsType);

	removeObsoleteNotifies(ARoster->streamJid(),AItemJid,ASubsType,false);
	notify.kinds = FNotifications!=NULL ? FNotifications->notificatorKinds(NID_SUBSCRIPTION) : 0;
	if (ASubsType==IRoster::Subscribed || ASubsType==IRoster::Unsubscribe)
		notify.kinds &= INotification::PopupWindow|INotification::PlaySound;
	if (notify.kinds > 0)
	{
		notify.notificatior = NID_SUBSCRIPTION;
		notify.data.insert(NDR_STREAM_JID,ARoster->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID,chatWindow!=NULL ? chatWindow->contactJid().full() : AItemJid.full());
		notify.data.insert(NDR_ICON_KEY,MNI_RCHANGER_SUBSCRIBTION);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_ORDER,RNO_RCHANGER_SUBSCRIPTION);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
		notify.data.insert(NDR_ROSTER_HOOK_CLICK,true);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
		notify.data.insert(NDR_ROSTER_FOOTER,notifyMessage);
		notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::magenta));
		notify.data.insert(NDR_TRAY_TOOLTIP,tr("%1 - authorization").arg(name.split(" ").value(0)));
		notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SUBSCRIPTION);
		notify.data.insert(NDR_TABPAGE_CREATE_TAB,true);
		notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
		notify.data.insert(NDR_TABPAGE_TOOLTIP,Qt::escape(notifyMessage));
		notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_RCHANGER_TABBARITEM_SUBSCRIPTION);
		notify.data.insert(NDR_POPUP_CAPTION,tr("Subscription"));
		notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AItemJid));
		notify.data.insert(NDR_POPUP_TITLE,name);
		notify.data.insert(NDR_POPUP_TEXT,Qt::escape(notifyMessage));
		notify.data.insert(NDR_POPUP_STYLEKEY, STS_RCHANGER_NOTIFYWIDGET_SUBSCRIPTION);
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
		notify.data.insert(NDR_SUBSCRIPTION_TYPE,ASubsType);
		notify.data.insert(NDR_SUBSCRIPTION_TEXT,AText);
	}

	int notifyId = -1;
	bool showNotice = false;
	int noticeActions = NTA_NO_ACTIONS;
	if (ASubsType == IRoster::Subscribe)
	{
		FSubscriptionRequests.insertMulti(ARoster->streamJid(),AItemJid);
		if (!isAutoSubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
		{
			if (FNotifications && notify.kinds>0)
			{
				notify.actions = createNotifyActions(NFA_SUBSCRIBE|NFA_UNSUBSCRIBE);
				notifyId = FNotifications->appendNotification(notify);
			}
			showNotice = true;
			noticeActions = NTA_SUBSCRIBE|NTA_UNSUBSCRIBE|NTA_CLOSE;
		}
		else
		{
			ARoster->sendSubscription(AItemJid,IRoster::Subscribed);
			if (isAutoSubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
				ARoster->sendSubscription(AItemJid,IRoster::Subscribe);
		}
	}
	else if (ASubsType == IRoster::Unsubscribed)
	{
		if (!isSilentSubsctiption(ARoster->streamJid(),AItemJid) && ritem.isValid)
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
			noticeActions = NTA_ASK_SUBSCRIBE|NTA_CLOSE;
		}

		if (isAutoUnsubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			ARoster->sendSubscription(AItemJid,IRoster::Unsubscribed);
	}
	else  if (ASubsType == IRoster::Subscribed)
	{
		if (!isSilentSubsctiption(ARoster->streamJid(),AItemJid))
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
		}
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		FSubscriptionRequests.remove(ARoster->streamJid(),AItemJid);
		if (!isSilentSubsctiption(ARoster->streamJid(),AItemJid) && ritem.isValid)
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
		}
	}

	int chatNoticeId = -1;
	chatWindow = chatWindow==NULL ? findChatNoticeWindow(ARoster->streamJid(),AItemJid) : chatWindow;
	removeObsoleteChatNotices(ARoster->streamJid(),AItemJid,ASubsType,false);
	if (chatWindow && showNotice)
	{
		if (noticeActions != NTA_NO_ACTIONS)
			chatNoticeId = insertChatNotice(chatWindow,createChatNotice(CNP_SUBSCRIPTION,noticeActions,notifyMessage,AText));
		else
			showNotifyInChatWindow(chatWindow,notifyMessage,AText);
	}
	else if (showNotice)
	{
		PendingChatNotice pnotice;
		pnotice.priority = CNP_SUBSCRIPTION;
		pnotice.notifyId = notifyId;
		pnotice.actions = noticeActions;
		pnotice.notify = notifyMessage;
		pnotice.text = AText;
		FPendingChatNotices[ARoster->streamJid()].insert(AItemJid.bare(),pnotice);
	}

	if (notifyId > 0)
	{
		foreach(Action *action, notify.actions)
		{
			action->setData(ADR_STREAM_JID,ARoster->streamJid().full());
			action->setData(ADR_CONTACT_JID,AItemJid.full());
			action->setData(ADR_NOTIFY_ID, notifyId);
			connect(action,SIGNAL(triggered(bool)),SLOT(onNotificationActionTriggered(bool)));
		}
		FNotifyChatNotice.insert(notifyId,chatNoticeId);
	}

	if (FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_TYPE,RIT_CONTACT);
		findData.insertMulti(RDR_BARE_JID,AItemJid.pBare());
		IRosterIndex *root = FRostersModel->streamRoot(ARoster->streamJid());
		foreach(IRosterIndex *index, root!=NULL ? root->findChild(findData,true) : QList<IRosterIndex *>())
		{
			emit rosterDataChanged(index,Qt::DecorationRole);
			emit rosterDataChanged(index,RDR_FOOTER_TEXT);
		}
	}
}

void RosterChanger::onAddItemToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid rosterJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_TO_GROUP).toString();
			IRosterItem ritem = roster->rosterItem(rosterJid);
			if (!ritem.isValid)
			{
				QString nick = action->data(ADR_NICK).toString();
				roster->setItem(rosterJid,nick,QSet<QString>()<<groupName);
			}
			else
			{
				roster->copyItemToGroup(rosterJid,groupName);
			}
		}
	}
}

void RosterChanger::onRenameItem(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid rosterJid = action->data(ADR_CONTACT_JID).toString();
			QString oldName = action->data(ADR_NICK).toString();
			bool ok = false;
			QString newName = QInputDialog::getText(NULL,tr("Contact name"),tr("Enter name for contact"), QLineEdit::Normal, oldName, &ok);
			if (ok && !newName.isEmpty() && newName != oldName)
				roster->renameItem(rosterJid, newName);
		}
	}
}

void RosterChanger::onCopyItemToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString rosterJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_TO_GROUP).toString();
			if (groupName.endsWith(groupDelim))
			{
				bool ok = false;
				QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"), QLineEdit::Normal,QString(),&ok);
				if (ok && !newGroupName.isEmpty())
				{
					if (groupName == groupDelim)
						groupName = newGroupName;
					else
						groupName+=newGroupName;
					roster->copyItemToGroup(rosterJid,groupName);
				}
			}
			else
				roster->copyItemToGroup(rosterJid,groupName);
		}
	}
}

void RosterChanger::onMoveItemToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString rosterJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_GROUP).toString();
			QString moveToGroup = action->data(ADR_TO_GROUP).toString();
			if (moveToGroup.endsWith(groupDelim))
			{
				bool ok = false;
				QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),QLineEdit::Normal,QString(),&ok);
				if (ok && !newGroupName.isEmpty())
				{
					if (moveToGroup == groupDelim)
						moveToGroup = newGroupName;
					else
						moveToGroup+=newGroupName;
					roster->moveItemToGroup(rosterJid,groupName,moveToGroup);
				}
			}
			else
				roster->moveItemToGroup(rosterJid,groupName,moveToGroup);
		}
	}
}

void RosterChanger::onRemoveItemFromGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString rosterJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_GROUP).toString();
			roster->removeItemFromGroup(rosterJid,groupName);
		}
	}
}

void RosterChanger::onRemoveItemFromRoster(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid rosterJid = action->data(ADR_CONTACT_JID).toString();
			if (roster->rosterItem(rosterJid).isValid)
			{
				if (QMessageBox::question(NULL,tr("Remove contact"),
							  tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(rosterJid.hBare()),
							  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				{
					roster->removeItem(rosterJid);
				}
			}
			else if (FRostersModel)
			{
				QMultiMap<int, QVariant> findData;
				findData.insert(RDR_TYPE,RIT_CONTACT);
				findData.insert(RDR_TYPE,RIT_AGENT);
				findData.insert(RDR_BARE_JID,rosterJid.pBare());
				IRosterIndex *streamIndex = FRostersModel->streamRoot(streamJid);
				foreach(IRosterIndex *index, streamIndex->findChild(findData,true))
					FRostersModel->removeRosterIndex(index);
			}
		}
	}
}

void RosterChanger::onChangeItemGroups(bool AChecked)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			IRosterItem ritem = roster->rosterItem(action->data(ADR_CONTACT_JID).toString());
			if (ritem.isValid)
			{
				bool checkBlank = false;
				bool uncheckBlank = false;
				bool uncheckGroups = false;
				QString group = action->data(ADR_TO_GROUP).toString();
				if (group == roster->groupDelimiter())
				{
					group = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));
					if (!group.isEmpty())
					{
						uncheckBlank = ritem.groups.isEmpty();
						roster->copyItemToGroup(ritem.itemJid,group);
					}
				}
				else if (group.isEmpty())
				{
					if (!ritem.groups.isEmpty())
					{
						uncheckGroups = true;
						roster->setItem(ritem.itemJid,ritem.name,QSet<QString>());
					}
					action->setChecked(true);
				}
				else if (AChecked)
				{
					uncheckBlank = ritem.groups.isEmpty();
					roster->copyItemToGroup(ritem.itemJid,group);
				}
				else
				{
					checkBlank = (ritem.groups-=group).isEmpty();
					roster->removeItemFromGroup(ritem.itemJid,group);
				}

				Menu *menu = qobject_cast<Menu *>(action->parent());
				if (menu && (checkBlank || uncheckBlank || uncheckGroups))
				{
					Action *blankAction = menu->groupActions(AG_DEFAULT-1).value(0);
					if (blankAction && checkBlank)
						blankAction->setChecked(true);
					else if (blankAction && uncheckBlank)
						blankAction->setChecked(false);

					foreach(Action *groupAction, menu->groupActions(AG_DEFAULT))
					{
						if (uncheckGroups)
							groupAction->setChecked(false);
					}
				}
			}
		}
	}
}

void RosterChanger::onAddGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString toStreamJid = action->data(ADR_STREAM_JID).toString();
		QString fromStreamJid = action->data(ADR_FROM_STREAM_JID).toString();
		IRoster *toRoster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(toStreamJid) : NULL;
		IRoster *fromRoster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(fromStreamJid) : NULL;
		if (fromRoster && toRoster && toRoster->isOpen())
		{
			QString toGroup = action->data(ADR_TO_GROUP).toString();
			QString fromGroup = action->data(ADR_GROUP).toString();
			QString fromGroupLast = fromGroup.split(fromRoster->groupDelimiter(),QString::SkipEmptyParts).last();

			QList<IRosterItem> toItems;
			QList<IRosterItem> fromItems = fromRoster->groupItems(fromGroup);
			foreach(IRosterItem fromItem, fromItems)
			{
				QSet<QString> newGroups;
				foreach(QString group, fromItem.groups)
				{
					if (group.startsWith(fromGroup))
					{
						QString newGroup = group;
						newGroup.remove(0,fromGroup.size());
						if (!toGroup.isEmpty())
							newGroup.prepend(toGroup + toRoster->groupDelimiter() + fromGroupLast);
						else
							newGroup.prepend(fromGroupLast);
						newGroups += newGroup;
					}
				}
				IRosterItem toItem = toRoster->rosterItem(fromItem.itemJid);
				if (!toItem.isValid)
				{
					toItem.isValid = true;
					toItem.itemJid = fromItem.itemJid;
					toItem.name = fromItem.name;
					toItem.groups = newGroups;
				}
				else
				{
					toItem.groups += newGroups;
				}
				toItems.append(toItem);
			}
			toRoster->setItems(toItems);
		}
	}
}

void RosterChanger::onRenameGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			bool ok = false;
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_GROUP).toString();
			QList<QString> groupTree = groupName.split(groupDelim,QString::SkipEmptyParts);

			QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),QLineEdit::Normal,groupTree.last(),&ok);
			if (ok && !newGroupPart.isEmpty())
			{
				QString newGroupName = groupName;
				newGroupName.chop(groupTree.last().size());
				newGroupName += newGroupPart;
				if (FEmptyGroups.contains(groupName))
				{
					IRosterIndex *index = FRostersModel!=NULL ? FRostersModel->findGroup(groupName,roster->groupDelimiter(),RIT_GROUP,FRostersModel->streamRoot(streamJid)) : NULL;
					if (index && !roster->groups().contains(newGroupName))
					{
						index->setData(RDR_INDEX_ID, newGroupPart);
						index->setData(RDR_GROUP,newGroupName);
						index->setData(RDR_NAME,newGroupPart);
						FEmptyGroups.removeAll(groupName);
						FEmptyGroups.append(newGroupName);
					}
				}
				else
				{
					roster->renameGroup(groupName,newGroupName);
				}
			}
		}
	}
}

void RosterChanger::onCopyGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_GROUP).toString();
			QString copyToGroup = action->data(ADR_TO_GROUP).toString();
			if (copyToGroup.endsWith(groupDelim))
			{
				bool ok = false;
				QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
						       QLineEdit::Normal,QString(),&ok);
				if (ok && !newGroupName.isEmpty())
				{
					if (copyToGroup == groupDelim)
						copyToGroup = newGroupName;
					else
						copyToGroup+=newGroupName;
					roster->copyGroupToGroup(groupName,copyToGroup);
				}
			}
			else
				roster->copyGroupToGroup(groupName,copyToGroup);
		}
	}
}

void RosterChanger::onMoveGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_GROUP).toString();
			QString moveToGroup = action->data(ADR_TO_GROUP).toString();
			if (moveToGroup.endsWith(roster->groupDelimiter()))
			{
				bool ok = false;
				QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
						       QLineEdit::Normal,QString(),&ok);
				if (ok && !newGroupName.isEmpty())
				{
					if (moveToGroup == groupDelim)
						moveToGroup = newGroupName;
					else
						moveToGroup+=newGroupName;
					roster->moveGroupToGroup(groupName,moveToGroup);
				}
			}
			else
				roster->moveGroupToGroup(groupName,moveToGroup);
		}
	}
}

void RosterChanger::onRemoveGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupName = action->data(ADR_GROUP).toString();
			if (FEmptyGroups.contains(groupName))
			{
				IRosterIndex *group = FRostersModel!=NULL ? FRostersModel->findGroup(groupName,roster->groupDelimiter(),RIT_GROUP,FRostersModel->streamRoot(streamJid)) : NULL;
				if (group)
				{
					group->instance()->deleteLater();
				}
			}
			else
				roster->removeGroup(groupName);
		}
	}
}

void RosterChanger::onRemoveGroupItems(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupName = action->data(ADR_GROUP).toString();
			QList<IRosterItem> ritems = roster->groupItems(groupName);
			if (ritems.count()>0 &&
			    QMessageBox::question(NULL,tr("Remove contacts"),
						  tr("You are assured that wish to remove %1 contact(s) from roster?").arg(ritems.count()),
						  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItems(ritems);
			}
		}
	}
}

void RosterChanger::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.subscription == SUBSCRIPTION_REMOVE)
	{
		if (isSilentSubsctiption(ARoster->streamJid(), AItem.itemJid))
			insertAutoSubscribe(ARoster->streamJid(), AItem.itemJid, true, false, false);
		else
			removeAutoSubscribe(ARoster->streamJid(), AItem.itemJid);
	}
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
	foreach(IChatWindow *window, FChatNoticeWindow.values())
	{
		if (window->streamJid() == ARoster->streamJid())
		{
			foreach(int chatNoticeId, FChatNoticeWindow.keys(window)) 
				window->noticeWidget()->removeNotice(chatNoticeId); 
		}
	}

	foreach(int notifyId, FNotifyChatNotice.keys())
	{
		INotification notify = FNotifications->notificationById(notifyId);
		if (ARoster->streamJid() == notify.data.value(NDR_STREAM_JID).toString())
			FNotifications->removeNotification(notifyId);
	}

	FPendingChatNotices.remove(ARoster->streamJid());
	FAutoSubscriptions.remove(ARoster->streamJid());
	FSubscriptionRequests.remove(ARoster->streamJid());
}

void RosterChanger::onEmptyGroupChildInserted(IRosterIndex *AIndex)
{
	Q_UNUSED(AIndex);
	IRosterIndex *group = qobject_cast<IRosterIndex *>(sender());
	if (group)
	{
		FEmptyGroups.removeAll(group->data(RDR_GROUP).toString());
		group->setData(RDR_ALLWAYS_VISIBLE, group->data(RDR_ALLWAYS_VISIBLE).toInt()-1);
		disconnect(group->instance(),SIGNAL(childInserted(IRosterIndex *)),this,SLOT(onEmptyGroupChildInserted(IRosterIndex *)));
		disconnect(group->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),this,SLOT(onEmptyGroupIndexDestroyed(IRosterIndex *)));
	}
}

void RosterChanger::onEmptyGroupIndexDestroyed(IRosterIndex *AIndex)
{
	FEmptyGroups.removeAll(AIndex->data(RDR_GROUP).toString());
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
	if (FNotifyChatNotice.contains(ANotifyId))
	{
		INotification notify = FNotifications->notificationById(ANotifyId);
		Jid streamJid = notify.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = notify.data.value(NDR_CONTACT_JID).toString();

		IChatWindow *window = FChatNoticeWindow.value(FNotifyChatNotice.value(ANotifyId));
		if (window)
		{
			window->showTabPage();
		}
		else if (FMessageProcessor==NULL || !FMessageProcessor->createWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW))
		{
			SubscriptionDialog *dialog = createSubscriptionDialog(streamJid,contactJid,notify.data.value(NDR_POPUP_TEXT).toString(),notify.data.value(NDR_SUBSCRIPTION_TEXT).toString());
			if (dialog)
				dialog->instance()->show();
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void RosterChanger::onNotificationRemoved(int ANotifyId)
{
		FNotifyChatNotice.remove(ANotifyId);
}

void RosterChanger::onNotificationActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int notifyId = action->data(ADR_NOTIFY_ID).toInt();
		if (FNotifications)
			FNotifications->removeNotification(notifyId);
	}
}

void RosterChanger::onChatWindowActivated()
{
	if (FNotifications)
	{
		IChatWindow *window = qobject_cast<IChatWindow *>(sender());
		if (window && !FPendingChatWindows.contains(window))
			removeNotifies(window);
	}
}

void RosterChanger::onChatWindowCreated(IChatWindow *AWindow)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AWindow->streamJid()) : NULL;
	IRosterItem ritem = roster!=NULL ? roster->rosterItem(AWindow->contactJid()) : IRosterItem();
	int pendingActions = FPendingChatNotices.value(AWindow->streamJid()).value(AWindow->contactJid().bare()).actions;
	if (roster && !ritem.isValid)
	{
		if (!AWindow->contactJid().node().isEmpty() && AWindow->streamJid().pBare()!=AWindow->contactJid().pBare())
			insertChatNotice(AWindow,createChatNotice(CNP_SUBSCRIPTION,NTA_ADD_CONTACT|NTA_CLOSE|pendingActions,tr("This contact is not added to your roster."),QString::null));
	}
	else if (roster && ritem.isValid)
	{
		if (ritem.ask!=SUBSCRIPTION_SUBSCRIBE && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
			insertChatNotice(AWindow,createChatNotice(CNP_SUBSCRIPTION,NTA_ASK_SUBSCRIBE|NTA_CLOSE|pendingActions,tr("Request authorization from contact to see his status and mood."),QString::null));
	}

	if (FPendingChatWindows.isEmpty())
		QTimer::singleShot(0,this,SLOT(onShowPendingChatNotices()));
	FPendingChatWindows.append(AWindow);

	connect(AWindow->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
	connect(AWindow->noticeWidget()->instance(),SIGNAL(noticeRemoved(int)),SLOT(onChatNoticeRemoved(int)));
}

void RosterChanger::onChatWindowDestroyed(IChatWindow *AWindow)
{
	FPendingChatWindows.removeAll(AWindow);
}

void RosterChanger::onShowPendingChatNotices()
{
	foreach(IChatWindow *window, FPendingChatWindows)
	{
		PendingChatNotice pnotice = FPendingChatNotices[window->streamJid()].take(window->contactJid().bare());
		if (pnotice.priority > 0)
		{
			int chatNoticeId = -1;
			if (pnotice.actions!=NTA_NO_ACTIONS && FChatNoticeWindow.key(window)<=0)
			{
				chatNoticeId = insertChatNotice(window,createChatNotice(pnotice.priority,pnotice.actions,pnotice.notify,pnotice.text));
				FNotifyChatNotice.insert(pnotice.notifyId,chatNoticeId);
			}
			else
			{
				showNotifyInChatWindow(window,pnotice.notify,pnotice.text);
			}
			if (window->isActive())
				removeNotifies(window);
		}
	}
	FPendingChatWindows.clear();
}

void RosterChanger::onChatNoticeActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int chatNoticeId = action->data(ADR_CHATNOTICE_ID).toInt();
		IChatWindow *window = FChatNoticeWindow.value(chatNoticeId);
		if (window)
			window->noticeWidget()->removeNotice(chatNoticeId);
	}
}

void RosterChanger::onChatNoticeRemoved(int ANoticeId)
{
	if (FNotifications)
		FNotifications->removeNotification(FNotifyChatNotice.key(ANoticeId));
	FChatNoticeWindow.remove(ANoticeId);
	FChatNoticeActions.remove(ANoticeId);
}

void RosterChanger::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Q_UNUSED(AWindow);
	if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AUser->data(MUDR_STREAM_JID).toString()) : NULL;
		if (roster && !roster->rosterItem(AUser->data(MUDR_REAL_JID).toString()).isValid)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Add contact"));
			action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REAL_JID));
			action->setData(ADR_NICK,AUser->data(MUDR_NICK_NAME));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_MUCM_ROSTERCHANGER,true);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_rosterchanger, RosterChanger)
