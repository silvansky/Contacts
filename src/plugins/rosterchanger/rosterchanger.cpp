#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
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
#define ADR_REQUEST         Action::DR_Parametr4
#define ADR_TO_GROUP        Action::DR_Parametr4
#define ADR_NOTICE_ID       Action::DR_UserDefined+1
#define ADR_NOTIFY_ID       Action::DR_UserDefined+2

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

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscription(IRoster *, const Jid &, int, const QString &)),
				SLOT(onReceiveSubscription(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemRemoved(IRoster *, const IRosterItem &)),
				SLOT(onRosterItemRemoved(IRoster *, const IRosterItem &)));
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
		uchar kindMask = INotification::RosterIcon|INotification::ChatWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound|INotification::AutoActivate;
		uchar kindDefs = INotification::RosterIcon|INotification::ChatWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound;
		FNotifications->insertNotificator(NID_SUBSCRIPTION,OWO_NOTIFICATIONS_SUBSCRIPTIONS,QString::null,kindMask,kindDefs);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	if (FMainWindowPlugin)
	{
		Menu *addMenu = new Menu(FMainWindowPlugin->mainWindow()->topToolBarChanger()->toolBar());
		addMenu->setTitle("Add...");
		addMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_CONTACT);

		Action *action = new Action(addMenu);
		action->setText(tr("Add contact"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_CONTACT);
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddContactDialog(bool)));
		addMenu->addAction(action);

		action = new Action(addMenu);
		action->setText(tr("Add group"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_RCHANGER_ADD_GROUP);
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddGroupDialog(bool)));
		addMenu->addAction(action);

		QToolButton *button = FMainWindowPlugin->mainWindow()->topToolBarChanger()->insertAction(addMenu->menuAction(), TBG_MWTTB_ROSTERCHANGER_ADDCONTACT);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		button->setDefaultAction(addMenu->menuAction());
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
		const IRosterItem &ritem = roster->rosterItem(AContactJid);
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
		const IRosterItem &ritem = roster->rosterItem(AContactJid);
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
		QSet<QString> allGroups = roster->groups();
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
						groupMenu->addAction(curGroupAction,AG_RVCM_ROSTERCHANGER+1);
					}

					if (ANewGroup)
					{
						Action *newGroupAction = new Action(groupMenu);
						newGroupAction->setText(tr("Create new..."));
						newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
						newGroupAction->setData(AData);
						newGroupAction->setData(ADR_TO_GROUP,groupName+groupDelim);
						connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
						groupMenu->addAction(newGroupAction,AG_RVCM_ROSTERCHANGER+1);
					}

					menus.insert(groupName,groupMenu);
					parentMenu->addAction(groupMenu->menuAction(),AG_RVCM_ROSTERCHANGER,true);
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
			menu->addAction(curGroupAction,AG_RVCM_ROSTERCHANGER+1);
		}

		if (ANewGroup)
		{
			Action *newGroupAction = new Action(menu);
			newGroupAction->setText(tr("Create new..."));
			newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
			newGroupAction->setData(AData);
			newGroupAction->setData(ADR_TO_GROUP,groupDelim);
			connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(newGroupAction,AG_RVCM_ROSTERCHANGER+1);
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

void RosterChanger::removeChatWindowNotifications(IChatWindow *AWindow)
{
	foreach(int noticeId, FNoticeWindow.keys(AWindow))
		FNotifications->removeNotification(FNotifyNotice.key(noticeId));
	FNotifications->removeNotification(FPendingNotice.value(AWindow->streamJid()).value(AWindow->contactJid().bare()).notifyId);
}

IChatWindow *RosterChanger::findNoticeWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IChatWindow *window, FNoticeWindow.values())
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

INotice RosterChanger::createNotice(int APriority, int AActions, const QString &ANotify, const QString &AText) const
{
	INotice notice;
	notice.priority = APriority;
	notice.iconKey = MNI_RCHANGER_SUBSCRIBTION;
	notice.iconStorage = RSR_STORAGE_MENUICONS;
	notice.message = !AText.isEmpty() ? Qt::escape(ANotify)+"<br>"+Qt::escape(AText) : Qt::escape(ANotify);

	if (AActions & NTA_ADD_CONTACT)
	{
		Action *addAction = new Action;
		addAction->setText(tr("Add Contact"));
		connect(addAction,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
		notice.actions.append(addAction);
	}
	if (AActions & NTA_ASK_SUBSCRIBE)
	{
		Action *askauthAction = new Action;
		askauthAction->setText(tr("Request authorization"));
		askauthAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		connect(askauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(askauthAction);
	}
	if (AActions & NTA_SUBSCRIBE)
	{
		Action *authAction = new Action;
		authAction->setText(tr("Authorize"));
		authAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		connect(authAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(authAction);
	}
	if (AActions & NTA_UNSUBSCRIBE)
	{
		Action *noauthAction = new Action;
		noauthAction->setText(tr("Don`t Authorize"));
		noauthAction->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
		connect(noauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(noauthAction);
	}
	if (AActions & NTA_CLOSE)
	{
		Action *closeAction = new Action;
		closeAction->setText(tr("Close"));
		notice.actions.append(closeAction);
	}

	return notice;
}

int RosterChanger::insertNotice(IChatWindow *AWindow, const INotice &ANotice)
{
	int noticeId = -1;
	if (AWindow && FMessageWidgets)
	{
		noticeId = AWindow->noticeWidget()->insertNotice(ANotice);
		foreach(Action *action, ANotice.actions)
		{
			action->setData(ADR_STREAM_JID,AWindow->streamJid().full());
			action->setData(ADR_CONTACT_JID,AWindow->contactJid().bare());
			action->setData(ADR_NOTICE_ID, noticeId);
			connect(action,SIGNAL(triggered(bool)),SLOT(onNoticeActionTriggered(bool)));
		}
		FNoticeWindow.insert(noticeId, AWindow);
	}
	return noticeId;
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
			dialog->setSubscriptionMessage(action->data(ADR_REQUEST).toString());
		}
	}
}

void RosterChanger::onShowAddGroupDialog(bool)
{
	bool ok = false;
	QString newGroupName = QInputDialog::getText(0, tr("Add group"), tr("Enter new group name:"), QLineEdit::Normal, "", &ok);
	if (ok && !newGroupName.isEmpty())
	{
		Jid streamJid = FAccountManager->accounts().first()->xmppStream()->streamJid();
		FRostersModel->createGroup(newGroupName, FRosterPlugin->getRoster(streamJid)->groupDelimiter(), RIT_GROUP, FRostersModel->streamRoot(streamJid));
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
			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_CONTACT_JID,AIndex->data(RDR_BARE_JID).toString());

			Menu *subsMenu = new Menu(AMenu);
			subsMenu->setTitle(tr("Subscription"));
			subsMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBTION);

			Action *action = new Action(subsMenu);
			action->setText(tr("Subscribe contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setText(tr("Unsubscribe contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setText(tr("Send"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_SEND);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Request"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REQUEST);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Remove"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REMOVE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Refuse"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REFUSE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			AMenu->addAction(subsMenu->menuAction(),AG_RVCM_ROSTERCHANGER_SUBSCRIPTION);

			action = new Action(AMenu);
			action->setText(tr("Remove from roster"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT);

			if (ritem.isValid)
			{
				QSet<QString> exceptGroups = ritem.groups;

				data.insert(ADR_NICK,AIndex->data(RDR_NAME));
				data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

				action = new Action(AMenu);
				action->setText(tr("Rename..."));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
				action->setData(data);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);

				if (itemType == RIT_CONTACT)
				{
					/*Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onCopyItemToGroup(bool)),AMenu);
					copyItem->setTitle(tr("Copy to group"));
					copyItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					AMenu->addAction(copyItem->menuAction(),AG_RVCM_ROSTERCHANGER);*/

					Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onMoveItemToGroup(bool)),AMenu);
					moveItem->setTitle(tr("Move to group"));
					moveItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
					AMenu->addAction(moveItem->menuAction(),AG_RVCM_ROSTERCHANGER);
				}

				if (!AIndex->data(RDR_GROUP).toString().isEmpty())
				{
					action = new Action(AMenu);
					action->setText(tr("Remove from group"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_FROM_GROUP);
					action->setData(data);
					connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromGroup(bool)));
					AMenu->addAction(action,AG_RVCM_ROSTERCHANGER);
				}
			}
			else
			{
				action = new Action(AMenu);
				action->setText(tr("Add contact"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				action->setData(ADR_STREAM_JID,streamJid);
				action->setData(ADR_CONTACT_JID,AIndex->data(RDR_JID));
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_ADD_CONTACT,true);
			}
		}
		else if (itemType == RIT_GROUP)
		{
			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

			Action *action = new Action(AMenu);
			action->setText(tr("Add contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_ADD_CONTACT,true);

			action = new Action(AMenu);
			action->setText(tr("Rename..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);

			QSet<QString> exceptGroups;
			exceptGroups << AIndex->data(RDR_GROUP).toString();

			Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onCopyGroupToGroup(bool)),AMenu);
			copyGroup->setTitle(tr("Copy to group"));
			copyGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
			AMenu->addAction(copyGroup->menuAction(),AG_RVCM_ROSTERCHANGER);

			Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onMoveGroupToGroup(bool)),AMenu);
			moveGroup->setTitle(tr("Move to group"));
			moveGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
			AMenu->addAction(moveGroup->menuAction(),AG_RVCM_ROSTERCHANGER);

			action = new Action(AMenu);
			action->setText(tr("Remove group"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_GROUP);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER);

			action = new Action(AMenu);
			action->setText(tr("Remove contacts"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACTS);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupItems(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER);
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

void RosterChanger::onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AText)
{
	INotification notify;
	IChatWindow *chatWindow = findNoticeWindow(ARoster->streamJid(),AContactJid);
	QString name = FNotifications->contactName(ARoster->streamJid(),AContactJid);
	QString notifyMessage = subscriptionNotify(ARoster->streamJid(),AContactJid,ASubsType);

	if (FNotifications)
	{
		notify.kinds = FNotifications->notificatorKinds(NID_SUBSCRIPTION);
		notify.notificatior = NID_SUBSCRIPTION;
		notify.data.insert(NDR_STREAM_JID,ARoster->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID,chatWindow!=NULL ? chatWindow->contactJid().full() : AContactJid.full());
		notify.data.insert(NDR_ICON_KEY,MNI_RCHANGER_SUBSCRIBTION);
		notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
		notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_SUBSCRIBTION);
		notify.data.insert(NDR_ROSTER_TOOLTIP,Qt::escape(notifyMessage));
		notify.data.insert(NDR_TRAY_TOOLTIP,tr("%1 - authorization").arg(name.split(" ").value(0)));
		notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SUBSCRIPTION);
		notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
		notify.data.insert(NDR_TABPAGE_TOOLTIP,Qt::escape(notifyMessage));
		notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_RCHANGER_TABBARITEM_SUBSCRIPTION);
		notify.data.insert(NDR_POPUP_CAPTION,tr("Subscription"));
		notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AContactJid));
		notify.data.insert(NDR_POPUP_TITLE,name);
		notify.data.insert(NDR_POPUP_TEXT,Qt::escape(notifyMessage));
		notify.data.insert(NDR_POPUP_STYLEKEY, STS_RCHANGER_NOTIFYWIDGET_SUBSCRIPTION);
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
		notify.data.insert(NDR_SUBSCRIPTION_TEXT,AText);
	}

	int notifyId = -1;
	bool showNotice = false;
	int noticeActions = NTA_NO_ACTIONS;
	const IRosterItem &ritem = ARoster->rosterItem(AContactJid);
	if (ASubsType == IRoster::Subscribe)
	{
		if (!isAutoSubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
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
			ARoster->sendSubscription(AContactJid,IRoster::Subscribed);
			if (isAutoSubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
				ARoster->sendSubscription(AContactJid,IRoster::Subscribe);
		}
	}
	else if (ASubsType == IRoster::Unsubscribed)
	{
		if (!isSilentSubsctiption(ARoster->streamJid(),AContactJid))
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
			noticeActions = NTA_ASK_SUBSCRIBE|NTA_CLOSE;
		}

		if (isAutoUnsubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			ARoster->sendSubscription(AContactJid,IRoster::Unsubscribed);
	}
	else  if (ASubsType == IRoster::Subscribed)
	{
		if (!isSilentSubsctiption(ARoster->streamJid(),AContactJid))
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
		}
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		if (!isSilentSubsctiption(ARoster->streamJid(),AContactJid))
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			showNotice = true;
		}
	}

	int noticeId = -1;
	if (showNotice && chatWindow)
	{
		if (noticeActions != NTA_NO_ACTIONS)
		{
			foreach(int noticeId, FNoticeWindow.keys(chatWindow))
				chatWindow->noticeWidget()->removeNotice(noticeId);
			noticeId = insertNotice(chatWindow,createNotice(NTP_SUBSCRIPTION,noticeActions,notifyMessage,AText));
		}
		else
		{
			noticeId = qrand()+1;
			showNotifyInChatWindow(chatWindow,notifyMessage,AText);
			FNoticeWindow.insert(noticeId,chatWindow);
		}
	}
	else if (showNotice)
	{
		PendingNotice pnotice;
		pnotice.priority = NTP_SUBSCRIPTION;
		pnotice.notifyId = notifyId;
		pnotice.actions = noticeActions;
		pnotice.notify = notifyMessage;
		pnotice.text = AText;
		FPendingNotice[ARoster->streamJid()].insert(AContactJid.bare(),pnotice);
	}

	if (notifyId > 0)
	{
		foreach(Action *action, notify.actions)
		{
			action->setData(ADR_STREAM_JID,ARoster->streamJid().full());
			action->setData(ADR_CONTACT_JID,AContactJid.full());
			action->setData(ADR_NOTIFY_ID, notifyId);
			connect(action,SIGNAL(triggered(bool)),SLOT(onNotificationActionTriggered(bool)));
		}
		FNotifyNotice.insert(notifyId,noticeId);
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
				QMultiHash<int, QVariant> data;
				data.insert(RDR_TYPE,RIT_CONTACT);
				data.insert(RDR_TYPE,RIT_AGENT);
				data.insert(RDR_BARE_JID,rosterJid.pBare());
				IRosterIndex *streamIndex = FRostersModel->streamRoot(streamJid);
				foreach(IRosterIndex *index, streamIndex->findChild(data,true))
					FRostersModel->removeRosterIndex(index);
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

			QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),
					       QLineEdit::Normal,groupTree.last(),&ok);

			if (ok && !newGroupPart.isEmpty())
			{
				QString newGroupName = groupName;
				newGroupName.chop(groupTree.last().size());
				newGroupName += newGroupPart;
				roster->renameGroup(groupName,newGroupName);
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

void RosterChanger::onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem)
{
	if (isSilentSubsctiption(ARoster->streamJid(), ARosterItem.itemJid))
		insertAutoSubscribe(ARoster->streamJid(), ARosterItem.itemJid, true, false, false);
	else
		removeAutoSubscribe(ARoster->streamJid(), ARosterItem.itemJid);
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
	FPendingNotice.remove(ARoster->streamJid());
	FAutoSubscriptions.remove(ARoster->streamJid());
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
	if (FNotifyNotice.contains(ANotifyId))
	{
		INotification notify = FNotifications->notificationById(ANotifyId);
		Jid streamJid = notify.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = notify.data.value(NDR_CONTACT_JID).toString();

		IChatWindow *window = FNoticeWindow.value(FNotifyNotice.value(ANotifyId));
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
	if (FNotifyNotice.contains(ANotifyId))
	{
		FNotifyNotice.remove(ANotifyId);
	}
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
			removeChatWindowNotifications(window);
	}
}

void RosterChanger::onChatWindowCreated(IChatWindow *AWindow)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AWindow->streamJid()) : NULL;
	IRosterItem ritem = roster!=NULL ? roster->rosterItem(AWindow->contactJid()) : IRosterItem();
	if (roster && !ritem.isValid)
	{
		if (!AWindow->contactJid().node().isEmpty() && AWindow->streamJid().pBare()!=AWindow->contactJid().pBare())
			insertNotice(AWindow,createNotice(NTP_SUBSCRIPTION,NTA_ADD_CONTACT|NTA_CLOSE,tr("This contact is not added to your roster."),QString::null));
	}
	else if (roster && ritem.isValid)
	{
		if (ritem.ask!=SUBSCRIPTION_SUBSCRIBE && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
			insertNotice(AWindow,createNotice(NTP_SUBSCRIPTION,NTA_ASK_SUBSCRIBE|NTA_CLOSE,tr("Request authorization from contact to see his status and mood."),QString::null));
	}

	if (FPendingChatWindows.isEmpty())
		QTimer::singleShot(0,this,SLOT(onShowPendingNotices()));
	FPendingChatWindows.append(AWindow);

	connect(AWindow->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
	connect(AWindow->noticeWidget()->instance(),SIGNAL(noticeRemoved(int)),SLOT(onNoticeRemoved(int)));
}

void RosterChanger::onChatWindowDestroyed(IChatWindow *AWindow)
{
	FPendingChatWindows.removeAll(AWindow);
}

void RosterChanger::onShowPendingNotices()
{
	foreach(IChatWindow *window, FPendingChatWindows)
	{
		PendingNotice pnotice = FPendingNotice[window->streamJid()].take(window->contactJid().bare());
		if (pnotice.priority > 0)
		{
			int noticeId = -1;
			if (pnotice.actions != NTA_NO_ACTIONS && FNoticeWindow.key(window)<=0)
			{
				noticeId = insertNotice(window,createNotice(pnotice.priority,pnotice.actions,pnotice.notify,pnotice.text));
				FNotifyNotice.insert(pnotice.notifyId,noticeId);
			}
			else
			{
				noticeId = qrand() + 1;
				showNotifyInChatWindow(window,pnotice.notify,pnotice.text);
				FNoticeWindow.insert(noticeId,window);
				FNotifyNotice.insert(pnotice.notifyId,noticeId);
			}
			if (window->isActive())
				removeChatWindowNotifications(window);
		}
	}
	FPendingChatWindows.clear();
}

void RosterChanger::onNoticeActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int noticeId = action->data(ADR_NOTICE_ID).toInt();
		IChatWindow *window = FNoticeWindow.take(noticeId);
		if (window)
			window->noticeWidget()->removeNotice(noticeId);
	}
}

void RosterChanger::onNoticeRemoved(int ANoticeId)
{
	if (FNotifications)
		FNotifications->removeNotification(FNotifyNotice.key(ANoticeId));
	FNoticeWindow.remove(ANoticeId);
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
