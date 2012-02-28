#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <definitions/customborder.h>
#include <definitions/statusicons.h>
#include <utils/customborderstorage.h>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_FROM_STREAM_JID Action::DR_Parametr2
#define ADR_SUBSCRIPTION    Action::DR_Parametr2
#define ADR_NICK            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_TO_GROUP        Action::DR_Parametr4
#define ADR_CONTACT_TEXT    Action::DR_Parametr4
#define ADR_CHATNOTICE_ID   Action::DR_UserDefined+1
#define ADR_NOTIFY_ID       Action::DR_UserDefined+2
#define ADR_NOTICE_ACTION   Action::DR_UserDefined+3

static const QList<int> DragGroups = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK;

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
	FGateways = NULL;
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = NULL;
	FRostersModel = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FXmppUriQueries = NULL;
	FAccountManager = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
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
	APluginInfo->homePage = "http://contacts.rambler.ru";
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

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());

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
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
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
			connect(FMessageWidgets->instance(),SIGNAL(viewWidgetCreated(IViewWidget *)),SLOT(onViewWidgetCreated(IViewWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_SUBSCRIPTIONS;
		notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::TabPageNotify|INotification::DockBadge|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_SUBSCRIPTION,notifyType);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
		FRostersView->insertEditHandler(REHO_ROSTERCHANGER_RENAME,this);
		FRostersView->insertKeyPressHooker(RCHO_DEFAULT,this);
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
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddGroupDialog(bool)));
		//mmenu->addAction(action,AG_MMENU_RCHAGER_ADD_GROUP);

		action = new Action(mmenu);
		action->setText(tr("Add contact..."));
		connect(action, SIGNAL(triggered(bool)), SLOT(onShowAddContactDialog(bool)));
		mmenu->addAction(action,AG_MMENU_RCHAGER_ADD_CONTACT);

		action = new Action(mmenu);
		action->setText(tr("Add account..."));
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
	return true;
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
			Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
			QString ask = AIndex->data(RDR_ASK).toString();
			QString subs = AIndex->data(RDR_SUBSCRIBTION).toString();
			if (FSubscriptionRequests.contains(streamJid,contactJid))
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool())
					{
						QVariantMap footer = AIndex->data(ARole).toMap();
						footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Requests authorization"));
						data = footer;
					}
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
				}
			}
			else if (ask == SUBSCRIPTION_SUBSCRIBE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool())
					{
						QVariantMap footer = AIndex->data(ARole).toMap();
						footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Sent an authorization request"));
						data = footer;
					}
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
				}
			}
			else if (subs == SUBSCRIPTION_NONE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool())
					{
						QVariantMap footer = AIndex->data(ARole).toMap();
						footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Not authorized"));
						data = footer;
					}
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
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
	if (AIndex.data(RDR_TYPE).toInt()==RIT_CONTACT && FRostersView->selectedRosterIndexes().count()<=1)
		return Qt::CopyAction|Qt::MoveAction;
	return Qt::IgnoreAction;
}

bool RosterChanger::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		stream >> indexData;

		if (indexData.value(RDR_TYPE).toInt() == RIT_CONTACT)
			return true;
	}
	return false;
}

bool RosterChanger::rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover)
{
	Q_UNUSED(AEvent);
	if (DragGroups.contains(AHover.data(RDR_TYPE).toInt()))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AHover.data(RDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen())
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
	if ((AEvent->dropAction() & (Qt::CopyAction|Qt::MoveAction))>0 && (DragGroups.contains(hoverType) || hoverType==RIT_STREAM_ROOT || hoverType==RIT_CONTACT))
	{
		Jid hoverStreamJid = AIndex.data(RDR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(hoverStreamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			stream >> indexData;

			int indexType = indexData.value(RDR_TYPE).toInt();
			Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
			bool isNewContact = indexType==RIT_CONTACT && !roster->rosterItem(indexData.value(RDR_PREP_BARE_JID).toString()).isValid;

			if (!isNewContact && (hoverStreamJid && indexStreamJid))
			{
				if (AEvent->dropAction() == Qt::CopyAction)
				{
					Action *copyAction = new Action(AMenu);
					copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
					copyAction->setData(ADR_TO_GROUP,AIndex.data(RDR_GROUP));
					if (indexType == RIT_CONTACT)
					{
						copyAction->setText(tr("Copy contact"));
						copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyItemToGroup(bool)));
						AMenu->addAction(copyAction,AG_DEFAULT,true);
					}
					else
					{
						copyAction->setText(tr("Copy group"));
						copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyGroupToGroup(bool)));
						AMenu->addAction(copyAction,AG_DEFAULT,true);
					}
					AMenu->setDefaultAction(copyAction);
					return true;
				}
				else if(AEvent->dropAction() == Qt::MoveAction)
				{
					Action *moveAction = new Action(AMenu);
					moveAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
					moveAction->setData(ADR_TO_GROUP,AIndex.data(RDR_GROUP));
					if (indexType == RIT_CONTACT)
					{
						moveAction->setText(tr("Move contact"));
						moveAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveItemToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					else
					{
						moveAction->setText(tr("Move group"));
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveGroupToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					AMenu->setDefaultAction(moveAction);
					return true;
				}
			}
			else
			{
				Action *copyAction = new Action(AMenu);
				copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				copyAction->setData(ADR_TO_GROUP,DragGroups.contains(hoverType) ? AIndex.data(RDR_GROUP) : QVariant(QString("")));
				if (indexType == RIT_CONTACT)
				{
					copyAction->setText(isNewContact ? tr("Add contact") : tr("Copy contact"));
					copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
					copyAction->setData(ADR_NICK,indexData.value(RDR_NAME));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onAddItemToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);
				}
				else
				{
					copyAction->setText(tr("Copy group"));
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

bool RosterChanger::rosterEditStart(int ADataRole, const QModelIndex &AIndex) const
{
	int type = AIndex.data(RDR_TYPE).toInt();
	if (ADataRole==RDR_NAME && (type==RIT_CONTACT || type==RIT_AGENT || type==RIT_GROUP))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
		return (roster && roster->isOpen());
	}
	return false;
}

QWidget * RosterChanger::rosterEditEditor(int ADataRole, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(AOption);
	Q_UNUSED(AIndex);
	if (ADataRole == RDR_NAME)
	{
		QLineEdit *editor = new QLineEdit(AParent);
		editor->setObjectName("lneEditIndex");
		editor->setAttribute(Qt::WA_MacShowFocusRect,false);
		editor->setFrame(false);
		return editor;
	}
	return NULL;
}

void RosterChanger::rosterEditLoadData(int ADataRole, QWidget *AEditor, const QModelIndex &AIndex) const
{
	if (ADataRole == RDR_NAME)
	{
		QLineEdit *editor = qobject_cast<QLineEdit *>(AEditor);
		if (editor)
			editor->setText(AIndex.data(RDR_NAME).toString());
	}
}

void RosterChanger::rosterEditSaveData(int ADataRole, QWidget *AEditor, const QModelIndex &AIndex) const
{
	int type = AIndex.data(RDR_TYPE).toInt();
	if (ADataRole==RDR_NAME && type==RIT_GROUP)
	{
		QLineEdit *editor = qobject_cast<QLineEdit *>(AEditor);
		QString newName = editor!=NULL ? editor->text().trimmed() : QString::null;
		if (!newName.isEmpty() && AIndex.data(RDR_NAME).toString()!=newName)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
			if (roster && roster->isOpen())
			{
				QString fullName = AIndex.data(RDR_GROUP).toString();
				fullName.chop(AIndex.data(RDR_NAME).toString().size());
				fullName += newName;

				IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(roster->streamJid()) : NULL;
				if (mroster && mroster->isOpen())
					mroster->renameGroup(AIndex.data(RDR_GROUP).toString(),fullName);
				else
					roster->renameGroup(AIndex.data(RDR_GROUP).toString(),fullName);
			}
		}
	}
	else if (ADataRole==RDR_NAME && (type==RIT_CONTACT || type==RIT_AGENT))
	{
		QLineEdit *editor = qobject_cast<QLineEdit *>(AEditor);
		QString newName = editor!=NULL ? editor->text().trimmed() : QString::null;
		if (!newName.isEmpty() && AIndex.data(RDR_NAME).toString()!=newName)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
			if (roster && roster->isOpen())
				roster->renameItem(AIndex.data(RDR_PREP_BARE_JID).toString(),newName);
		}
	}
}

void RosterChanger::rosterEditGeometry(int ADataRole, QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	if (ADataRole == RDR_NAME)
	{
		QRect rect = FRostersView->labelRect(RLID_DISPLAY_EDIT,AIndex);
		if (rect.isValid())
			AEditor->setGeometry(rect);
		else
			AEditor->setGeometry(AOption.rect);
	}
}

bool RosterChanger::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "roster")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && !roster->rosterItem(AContactJid).isValid)
		{
			IAddContactDialog * dialog = NULL;
			QWidget * widget = showAddContactDialog(AStreamJid);
			if (widget)
			{
				if (!(dialog = qobject_cast<IAddContactDialog*>(widget)))
				{
					if (CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(widget))
						dialog = qobject_cast<IAddContactDialog*>(border->widget());
				}
				if (dialog)
				{
					dialog->setContactJid(AContactJid);
					dialog->setNickName(AParams.contains("name") ? AParams.value("name") : AContactJid.node());
					dialog->setGroup(AParams.contains("group") ? AParams.value("group") : QString::null);
					dialog->instance()->show();
				}
			}
		}
		return true;
	}
	return false;
}

// TODO: implement following 2 functions to rename / delete contacts on F2/DEL keys
bool RosterChanger::keyOnRosterIndexPressed(IRosterIndex *AIndex, int AOrder, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
	Q_UNUSED(AOrder);
	if (AIndex->type()==RIT_CONTACT && ((key == Qt::Key_Delete) || (key == Qt::Key_Backspace)) && (modifiers == Qt::NoModifier))
	{
		Action *remove = new Action;
		connect(remove,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));

		QHash<int,QVariant> data;
		data.insert(ADR_STREAM_JID,AIndex->data(RDR_STREAM_JID));
		data.insert(ADR_CONTACT_JID,AIndex->data(RDR_FULL_JID));
		remove->setData(data);
		remove->trigger();
		remove->deleteLater();

		return true;
	}
	return false;
}

bool RosterChanger::keyOnRosterIndexesPressed(IRosterIndex *AIndex, QList<IRosterIndex*> ASelected, int AOrder, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
	Q_UNUSED(AOrder)
	Q_UNUSED(AIndex)
	Q_UNUSED(ASelected)
	Q_UNUSED(key)
	Q_UNUSED(modifiers)
	return false;
}

//IRosterChanger
bool RosterChanger::isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FGateways && AContactJid.node().isEmpty() && FGateways->availServices(AStreamJid).contains(AContactJid))
		return true;
	else if (Options::node(OPV_ROSTER_AUTOSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoSubscribe;
	return false;
}

bool RosterChanger::isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FGateways && AContactJid.node().isEmpty() && FGateways->availServices(AStreamJid).contains(AContactJid))
		return true;
	else if (Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoUnsubscribe;
	return false;
}

bool RosterChanger::isSilentSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
	{
		AutoSubscription asubscr = FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare());
		return asubscr.silent && asubscr.autoSubscribe;
	}
	else if (FGateways && AContactJid.node().isEmpty() && FGateways->availServices(AStreamJid).contains(AContactJid))
	{
		return true;
	}
	return false;
}

bool RosterChanger::isSilentUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
	{
		AutoSubscription asubscr = FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare());
		return asubscr.silent && asubscr.autoUnsubscribe;
	}
	else if (FGateways && AContactJid.node().isEmpty() && FGateways->availServices(AStreamJid).contains(AContactJid))
	{
		return true;
	}
	return false;
}

void RosterChanger::insertAutoSubscribtion(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr)
{
	AutoSubscription &asubscr = FAutoSubscriptions[AStreamJid][AContactJid.bare()];
	asubscr.silent = ASilently;
	asubscr.autoSubscribe = ASubscr;
	asubscr.autoUnsubscribe = AUnsubscr;
}

void RosterChanger::removeAutoSubscribtion(const Jid &AStreamJid, const Jid &AContactJid)
{
	FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		LogDetail(QString("[RosterChanger] Subscribing contact '%1'").arg(AContactJid.bare()));
		IRosterItem ritem = roster->rosterItem(AContactJid);
		if (FSubscriptionRequests.contains(AStreamJid,AContactJid.bare()))
			roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
		insertAutoSubscribtion(AStreamJid,AContactJid,ASilently,true,false);
	}
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		LogDetail(QString("[RosterChanger] Unsubscribing contact '%1'").arg(AContactJid.bare()));
		IRosterItem ritem = roster->rosterItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Unsubscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_NONE)
			roster->sendSubscription(AContactJid,IRoster::Unsubscribe,AMessage);
		insertAutoSubscribtion(AStreamJid,AContactJid,ASilently,false,true);
	}
}

IAddMetaItemWidget *RosterChanger::newAddMetaItemWidget(const Jid &AStreamJid, const QString &AGateDescriptorId, QWidget *AParent)
{
	IAddMetaItemWidget *widget = NULL;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (FGateways && roster)
	{
		IGateServiceDescriptor descriptor = FGateways->gateDescriptorById(AGateDescriptorId);
		if (!descriptor.id.isEmpty() && !(descriptor.needGate && descriptor.readOnly))
		{
			widget = new AddMetaItemWidget(FOptionsManager,roster,FGateways,descriptor,AParent);
			emit addMetaItemWidgetCreated(widget);
		}
	}
	return widget;
}

QWidget *RosterChanger::showAddContactDialog(const Jid &AStreamJid)
{
	IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QDialog *dialog;
		//IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(roster->streamJid()) : NULL;
		//if (mroster!=NULL && mroster->isOpen())
		//	dialog = new AddMetaContactDialog(mroster,this,FPluginManager);
		//else
			dialog = new AddContactDialog(roster,this,FPluginManager);
		connect(roster->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
		emit addContactDialogCreated(qobject_cast<IAddContactDialog *>(dialog));

		CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
		if (border)
		{
			border->setAttribute(Qt::WA_DeleteOnClose, true);
			border->setMaximizeButtonVisible(false);
			border->setMinimizeButtonVisible(false);
			border->setResizable(false);
			connect(border, SIGNAL(closeClicked()), dialog, SLOT(reject()));
			connect(dialog, SIGNAL(rejected()), border, SLOT(close()));
			connect(dialog, SIGNAL(accepted()), border, SLOT(close()));
			border->show();
		}
		else
		{
			dialog->show();
		}
		return border ? (QWidget*)border : (QWidget*)dialog;
	}
	return NULL;
}

QString RosterChanger::contactName(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FNotifications!=NULL ? FNotifications->contactName(AStreamJid,AContactJid) : (FGateways!=NULL ? FGateways->legacyIdFromUserJid(AStreamJid,AContactJid) : AContactJid.bare());
}

QString RosterChanger::subscriptionNotify(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType) const
{
	Q_UNUSED(AStreamJid);
	Q_UNUSED(AContactJid);
	switch (ASubsType)
	{
	case IRoster::Subscribe:
		return tr("Requests authorization");
	case IRoster::Subscribed:
		return tr("Confirmed authorization");
	case IRoster::Unsubscribe:
		return tr("Declined authorization");
	case IRoster::Unsubscribed:
		return tr("Refused to authorize you");
	}
	return QString::null;
}

QString RosterChanger::subscriptionNotice(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType) const
{
	QString name = contactName(AStreamJid,AContactJid);
	switch (ASubsType)
	{
	case IRoster::Subscribe:
		return tr("%1 requests authorization (permission to see your status and mood).").arg(name);
	case IRoster::Subscribed:
		return tr("%1 has confirmed your authorization request, now you can see its status and mood.").arg(name);
	case IRoster::Unsubscribe:
		return tr("%1 has declined from your authorization, now he can't see your status ans mood.").arg(name);
	case IRoster::Unsubscribed:
		return tr("%1 refused to authorize you, now you can't see its status and mood.").arg(name);
	}
	return QString::null;
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
	notice.message = !AText.isEmpty() ? Qt::escape(ANotify)+"<br>"+Qt::escape(AText) : Qt::escape(ANotify);

	if (AActions & NTA_ADD_CONTACT)
	{
		Action *addAction = new Action;
		addAction->setText(tr("Add contact"));
		addAction->setData(ADR_NOTICE_ACTION,NTA_ADD_CONTACT);
		addAction->setProperty("actionName", "addRequest");
		connect(addAction,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
		notice.actions.append(addAction);
	}
	if (AActions & NTA_ASK_SUBSCRIBE)
	{
		Action *askauthAction = new Action;
		askauthAction->setText(tr("Request authorization"));
		askauthAction->setData(ADR_NOTICE_ACTION,NTA_ASK_SUBSCRIBE);
		askauthAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		askauthAction->setProperty("actionName", "authRequest");
		connect(askauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(askauthAction);
	}
	if (AActions & NTA_SUBSCRIBE)
	{
		Action *authAction = new Action;
		authAction->setText(tr("Authorize"));
		authAction->setData(ADR_NOTICE_ACTION,NTA_SUBSCRIBE);
		authAction->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
		authAction->setProperty("actionName", "authorize");
		connect(authAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(authAction);
	}
	if (AActions & NTA_UNSUBSCRIBE)
	{
		Action *noauthAction = new Action;
		noauthAction->setText(tr("Reject"));
		noauthAction->setData(ADR_NOTICE_ACTION,NTA_UNSUBSCRIBE);
		noauthAction->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
		noauthAction->setProperty("actionName", "rejectAuthRequest");
		connect(noauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		notice.actions.append(noauthAction);
	}
	if (AActions & NTA_CLOSE)
	{
		Action *closeAction = new Action;
		closeAction->setText(tr("Close"));
		closeAction->setData(ADR_NOTICE_ACTION,NTA_CLOSE);
		closeAction->setProperty("actionName", "close");
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

void RosterChanger::removeWindowChatNotices(IChatWindow *AWindow)
{
	foreach(int noticeId, FChatNoticeWindow.keys(AWindow))
		AWindow->noticeWidget()->removeNotice(noticeId);
}

bool RosterChanger::isObsoleteChatNotice(int AActions, int ASubsType, bool ASent)
{
	bool remove = false;
	if (ASubsType == IRoster::Subscribe)
	{
		if (ASent)
			remove = (AActions & (NTA_ADD_CONTACT|NTA_ASK_SUBSCRIBE))>0;
	}
	else if (ASubsType == IRoster::Subscribed)
	{
		if (ASent)
			remove = (AActions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
		else
			remove = (AActions & (NTA_ADD_CONTACT|NTA_ASK_SUBSCRIBE))>0;
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		if (ASent)
			remove = (AActions & NTA_ASK_SUBSCRIBE)>0;
		else
			remove = (AActions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
	}
	else if (ASubsType == IRoster::Unsubscribed)
	{
		if (ASent)
			remove = (AActions & (NTA_ADD_CONTACT|NTA_SUBSCRIBE|NTA_UNSUBSCRIBE))>0;
	}
	return remove;
}

void RosterChanger::removeObsoleteChatNotices(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent)
{
	foreach(IChatWindow *window, FChatNoticeWindow.values())
	{
		if (window->streamJid()==AStreamJid && (window->contactJid() && AContactJid))
		{
			int chatNoticeId = FChatNoticeWindow.key(window);
			if (isObsoleteChatNotice(FChatNoticeActions.value(chatNoticeId),ASubsType,ASent))
				window->noticeWidget()->removeNotice(chatNoticeId);
		}
	}

	QMap<Jid, PendingChatNotice> &notices = FPendingChatNotices[AStreamJid];
	QMap<Jid, PendingChatNotice>::iterator it = notices.begin();
	while(it != notices.end())
	{
		if ((it.key() && AContactJid) && isObsoleteChatNotice(it->actions,ASubsType,ASent))
			it = notices.erase(it);
		else
			it++;
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
		authAction->setData(Action::DR_UserDefined + 1, "authorize");
		connect(authAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		actions.append(authAction);
	}
	if (AActions & NFA_UNSUBSCRIBE)
	{
		Action *noauthAction = new Action;
		noauthAction->setText(tr("Cancel"));
		noauthAction->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
		noauthAction->setData(Action::DR_UserDefined + 1, "cancel");
		connect(noauthAction,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
		actions.append(noauthAction);
	}
	if (AActions & NFA_CLOSE)
	{
		Action *closeAction = new Action;
		closeAction->setText(tr("Close"));
		closeAction->setData(Action::DR_UserDefined + 1, "close");
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
	options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;

	QString message = !AText.isEmpty() ? ANotify +" (" +AText+ ")" : ANotify;
	AWindow->viewWidget()->changeContentText(message,options);
}

void RosterChanger::onShowAddContactDialog(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IAccount *account = FAccountManager ? FAccountManager->accounts().first() : NULL;
	if (action && account && account->isActive())
	{
		IAddContactDialog * dialog = NULL;
		QWidget *widget = showAddContactDialog(account->xmppStream()->streamJid());
		if (widget)
		{
			if (!(dialog = qobject_cast<IAddContactDialog*>(widget)))
			{
				if (CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(widget))
					dialog = qobject_cast<IAddContactDialog*>(border->widget());
			}
			if (dialog)
			{
				dialog->setNickName(action->data(ADR_NICK).toString());
				dialog->setGroup(action->data(ADR_GROUP).toString());
				if (!action->data(ADR_CONTACT_JID).toString().isEmpty())
				{
					dialog->setContactJid(action->data(ADR_CONTACT_JID).toString());
					dialog->executeRequiredContactChecks();
				}
				else if (!action->data(ADR_CONTACT_TEXT).toString().isEmpty())
				{
					dialog->setContactText(action->data(ADR_CONTACT_TEXT).toString());
				}
			}
		}
	}
}

void RosterChanger::onShowAddGroupDialog(bool)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accounts().value(0) : NULL;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(account!=NULL ? account->xmppStream()->streamJid() : Jid::null) : NULL;
	if (FRostersModel && roster)
	{
		QInputDialog * dialog = new QInputDialog;
		dialog->setInputMode(QInputDialog::TextInput);
		dialog->setLabelText(tr("<font size=+2>Add group</font><br>Enter new group name:"));
		dialog->setWindowTitle(tr("Add group"));
		//QString newGroupName = QInputDialog::getText(NULL, tr("Add group"), tr("Enter new group name:"));
		connect(dialog, SIGNAL(textValueSelected(QString)), SLOT(onRenameGroupDialogAccepted(QString)));
		CustomBorderContainer * border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
		if (border)
		{
			border->setAttribute(Qt::WA_DeleteOnClose, true);
			border->setMaximizeButtonVisible(false);
			border->setMinimizeButtonVisible(false);
			connect(border, SIGNAL(closeClicked()), dialog, SLOT(reject()));
			connect(dialog, SIGNAL(rejected()), border, SLOT(close()));
			connect(dialog, SIGNAL(accepted()), border, SLOT(close()));
			border->setResizable(false);
			border->show();
		}
		else
		{
			dialog->show();
		}
	}
}

void RosterChanger::onRenameGroupDialogAccepted(QString ANewGroupName)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accounts().value(0) : NULL;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(account!=NULL ? account->xmppStream()->streamJid() : Jid::null) : NULL;
	if (sender()->property("rename").toBool())
	{
		if (!ANewGroupName.isEmpty())
		{
			QString groupName = sender()->property("groupName").toString();
			QString streamJid = sender()->property("streamJid").toString();
			QStringList groupTree = sender()->property("groupTree").toStringList();
			QString completeGroupName = groupName;
			completeGroupName.chop(groupTree.last().size());
			completeGroupName += ANewGroupName;

			if (FEmptyGroups.contains(groupName))
			{
				IRosterIndex *index = FRostersModel!=NULL ? FRostersModel->findGroupIndex(RIT_GROUP,groupName,roster->groupDelimiter(),FRostersModel->streamRoot(streamJid)) : NULL;
				if (index && !roster->groups().contains(completeGroupName))
				{
					index->setData(RDR_GROUP,completeGroupName);
					index->setData(RDR_NAME,ANewGroupName);
					FEmptyGroups.removeAll(groupName);
					FEmptyGroups.append(completeGroupName);
				}
			}
			else
			{
				IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(roster->streamJid()) : NULL;
				if (mroster && mroster->isOpen())
					mroster->renameGroup(groupName,completeGroupName);
				else
					roster->renameGroup(groupName,completeGroupName);
			}
		}
	}
	else
	{
		if (FRostersModel && roster && !ANewGroupName.isEmpty() && !ANewGroupName.contains(roster->groupDelimiter()) && FRostersModel->findGroupIndex(RIT_GROUP,ANewGroupName,roster->groupDelimiter(),FRostersModel->streamRoot(roster->streamJid()))==NULL)
		{
			IRosterIndex *group = FRostersModel->createGroupIndex(RIT_GROUP,ANewGroupName,roster->groupDelimiter(),FRostersModel->streamRoot(roster->streamJid()));
			if (group)
			{
				FEmptyGroups.append(ANewGroupName);
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
		FOptionsManager->showOptionsDialog(OPN_GATEWAYS);
	}
}

void RosterChanger::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(streamJid) : NULL;
	if (roster && roster->isOpen() && ASelected.count()<2)
	{
		int itemType = AIndex->data(RDR_TYPE).toInt();
		if (itemType == RIT_CONTACT || itemType == RIT_AGENT)
		{
			QString contactJid = AIndex->data(RDR_FULL_JID).toString();

			bool canEdit = true;
			if (FGateways)
			{
				IGateServiceDescriptor descriptor = FGateways->serviceDescriptor(streamJid,Jid(contactJid).domain());
				if (!descriptor.readOnly)
				{
					Jid itemJid = contactJid;
					if (!itemJid.node().isEmpty() && FGateways->streamServices(streamJid).contains(itemJid.domain()))
					{
						IPresenceItem pitem = FGateways->servicePresence(streamJid,itemJid.domain());
						if (pitem.show==IPresence::Offline || pitem.show==IPresence::Error)
							canEdit = false;
					}
				}
				else
				{
					canEdit = false;
				}
			}

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_CONTACT_JID,contactJid);

			IRosterItem ritem = roster->rosterItem(contactJid);
			if (FSubscriptionRequests.contains(streamJid,contactJid))
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Authorize"));
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_GRAND_AUTH);

				action = new Action(AMenu);
				action->setText(tr("Refuse authorization"));
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_AUTH);
			}
			else if (ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO && ritem.ask!=SUBSCRIPTION_SUBSCRIBE)
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Request authorization"));
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
				connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_GRAND_AUTH);
			}

			Action *action = new Action(AMenu);
			action->setText(tr("Delete"));
			action->setData(data);
			action->setEnabled(canEdit);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT);

			if (ritem.isValid)
			{
				data.insert(ADR_NICK,AIndex->data(RDR_NAME));
				data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

				action = new Action(AMenu);
				action->setText(tr("Rename..."));
				action->setData(data);
				action->setEnabled(canEdit);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);

				if (AIndex->type() == RIT_CONTACT)
				{
					GroupMenu *groupMenu = new GroupMenu(AMenu);
					groupMenu->setTitle(tr("Group"));
					groupMenu->setEnabled(canEdit);

					Action *blankGroupAction = new Action(groupMenu);
					blankGroupAction->setText(FRostersModel->singleGroupName(RIT_GROUP_BLANK));
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
				action->setData(ADR_STREAM_JID,streamJid);
				action->setData(ADR_CONTACT_JID,contactJid);
				action->setEnabled(canEdit);
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
				AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_ADD_CONTACT);
			}
		}
		else if (itemType == RIT_GROUP)
		{
			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

			bool canEdit = false;
			foreach(IRosterItem groupItem, roster->groupItems(AIndex->data(RDR_GROUP).toString()))
			{
				IGateServiceDescriptor descriptor = FGateways!=NULL ? FGateways->serviceDescriptor(streamJid,groupItem.itemJid.domain()) : IGateServiceDescriptor();
				if (!descriptor.readOnly)
				{
					canEdit = true;
					break;
				}
			}

			Action *action = new Action(AMenu);
			action->setText(tr("Rename group..."));
			action->setData(data);
			action->setEnabled(canEdit);
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
			int subsType = action->data(ADR_SUBSCRIPTION).toInt();
			if (subsType == IRoster::Subscribe)
				subscribeContact(streamJid,contactJid,QString::null,true);
			else if (subsType == IRoster::Unsubscribe)
				unsubscribeContact(streamJid,contactJid,QString::null,true);
		}
	}
}

void RosterChanger::onSendSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
			int subsType = action->data(ADR_SUBSCRIPTION).toInt();
			roster->sendSubscription(contactJid,subsType);
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
	QString name = contactName(ARoster->streamJid(),AItemJid);
	IChatWindow *chatWindow = findChatNoticeWindow(ARoster->streamJid(),AItemJid);
	QString notifyMessage = subscriptionNotify(ARoster->streamJid(),AItemJid,ASubsType);
	QString noticeMessage = subscriptionNotice(ARoster->streamJid(),AItemJid,ASubsType);
	bool isService = FMetaContacts!=NULL ? FMetaContacts->metaDescriptorByItem(AItemJid).service : false;

	removeObsoleteNotifies(ARoster->streamJid(),AItemJid,ASubsType,false);
	notify.kinds = !isService && FNotifications!=NULL ? FNotifications->enabledTypeNotificationKinds(NNT_SUBSCRIPTION) : 0;
	if (ASubsType==IRoster::Subscribed || ASubsType==IRoster::Unsubscribe)
		notify.kinds &= INotification::PopupWindow|INotification::SoundPlay;

	if (notify.kinds > 0)
	{
		notify.typeId = NNT_SUBSCRIPTION;
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
		notify.data.insert(NDR_POPUP_TITLE, name);
		notify.data.insert(NDR_POPUP_NOTICE, notifyMessage);
		notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(ARoster->streamJid(),AItemJid));
#ifdef Q_WS_MAC
		notify.data.insert(NDR_POPUP_TEXT,Qt::escape(noticeMessage));
#endif
		notify.data.insert(NDR_POPUP_STYLEKEY, STS_RCHANGER_NOTIFYWIDGET_SUBSCRIPTION);
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
		notify.data.insert(NDR_SUBSCRIPTION_TYPE,ASubsType);
		notify.data.insert(NDR_SUBSCRIPTION_TEXT,AText);

		if (chatWindow)
		{
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)chatWindow->instance());
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)chatWindow->instance());
			notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)chatWindow->instance());
			notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_SUBSCRIPTION);
			notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,1);
			notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
			notify.data.insert(NDR_TABPAGE_TOOLTIP, Qt::escape(noticeMessage));
			notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_RCHANGER_TABBARITEM_SUBSCRIPTION);
		}
	}

	int notifyId = -1;
	bool showNotice = false;
	int noticeActions = NTA_NO_ACTIONS;
	if (ASubsType == IRoster::Subscribe)
	{
		FSubscriptionRequests.insertMulti(ARoster->streamJid(),AItemJid);
		if (!isAutoSubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
		{
			if (!isService)
			{
				if (FNotifications && notify.kinds>0)
				{
					notify.actions = createNotifyActions(NFA_SUBSCRIBE|NFA_UNSUBSCRIBE);
					foreach(Action *action, notify.actions)
					{
						action->setData(ADR_STREAM_JID,ARoster->streamJid().full());
						action->setData(ADR_CONTACT_JID,AItemJid.full());
						action->setData(ADR_NOTIFY_ID, notifyId);
						connect(action,SIGNAL(triggered(bool)),SLOT(onNotificationActionTriggered(bool)));
					}
					notifyId = FNotifications->appendNotification(notify);
				}
				showNotice = true;
				if (ritem.isValid)
					noticeActions = NTA_SUBSCRIBE|NTA_UNSUBSCRIBE;
				else
					noticeActions = NTA_ADD_CONTACT|NTA_UNSUBSCRIBE;
			}
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
		if (!isService && !isSilentUnsubscribe(ARoster->streamJid(),AItemJid) && ritem.isValid)
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
			noticeActions = NTA_ASK_SUBSCRIBE|NTA_CLOSE;
		}
		showNotice = !isService;

		if (isAutoUnsubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_NONE)
			ARoster->sendSubscription(AItemJid,IRoster::Unsubscribed);
	}
	else if (ASubsType == IRoster::Subscribed)
	{
		if (!isService && !isSilentSubscribe(ARoster->streamJid(),AItemJid))
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
		}
		showNotice = !isService;
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		FSubscriptionRequests.remove(ARoster->streamJid(),AItemJid);
		if (!isService && !isSilentUnsubscribe(ARoster->streamJid(),AItemJid) && ritem.isValid)
		{
			if (FNotifications && notify.kinds>0)
				notifyId = FNotifications->appendNotification(notify);
		}
		showNotice = !isService;
	}

	int chatNoticeId = -1;
	removeObsoleteChatNotices(ARoster->streamJid(),AItemJid,ASubsType,false);
	if (showNotice)
	{
		chatWindow = chatWindow==NULL ? findChatNoticeWindow(ARoster->streamJid(),AItemJid) : chatWindow;
		if (chatWindow)
		{
			if (noticeActions != NTA_NO_ACTIONS)
			{
				removeWindowChatNotices(chatWindow);
				chatNoticeId = insertChatNotice(chatWindow,createChatNotice(CNP_SUBSCRIPTION,noticeActions,noticeMessage,AText));
			}
			showNotifyInChatWindow(chatWindow,noticeMessage,AText);
		}
		else
		{
			PendingChatNotice pnotice;
			pnotice.priority = CNP_SUBSCRIPTION;
			pnotice.notifyId = notifyId;
			pnotice.actions = noticeActions;
			pnotice.notify = noticeMessage;
			pnotice.text = AText;
			FPendingChatNotices[ARoster->streamJid()].insert(AItemJid.bare(),pnotice);
		}
	}

	if (notifyId > 0)
	{
		FNotifyChatNotice.insert(notifyId,chatNoticeId);
	}

	if (FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_TYPE,RIT_CONTACT);
		findData.insertMulti(RDR_PREP_BARE_JID,AItemJid.pBare());
		IRosterIndex *root = FRostersModel->streamRoot(ARoster->streamJid());
		foreach(IRosterIndex *index, root!=NULL ? root->findChilds(findData,true) : QList<IRosterIndex *>())
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid contactJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_TO_GROUP).toString();
			IRosterItem ritem = roster->rosterItem(contactJid);
			if (!ritem.isValid)
			{
				QString nick = action->data(ADR_NICK).toString();
				roster->setItem(contactJid,nick,QSet<QString>()<<groupName);
			}
			else
			{
				roster->copyItemToGroup(contactJid,groupName);
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
		IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid contactJid = action->data(ADR_CONTACT_JID).toString();
			if (FRostersView && FRostersView->rostersModel())
			{
				QString group = action->data(ADR_GROUP).toString();
				QList<IRosterIndex *> indexes = FRostersView->rostersModel()->getContactIndexList(streamJid,contactJid);
				foreach(IRosterIndex *index, indexes)
				{
					if (index->data(RDR_GROUP).toString() == group)
					{
						FRostersView->editRosterIndex(RDR_NAME,index);
						break;
					}
				}
			}
			else
			{
				bool ok = false;
				QString oldName = action->data(ADR_NICK).toString();
				QString newName = QInputDialog::getText(NULL,tr("Contact name"),tr("Enter name for contact"), QLineEdit::Normal, oldName, &ok);
				if (ok && !newName.isEmpty() && newName != oldName)
					roster->renameItem(contactJid, newName);
			}
		}
	}
}

void RosterChanger::onCopyItemToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_TO_GROUP).toString();
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
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
					roster->copyItemToGroup(contactJid,groupName);
				}
			}
			else
				roster->copyItemToGroup(contactJid,groupName);
		}
	}
}

void RosterChanger::onMoveItemToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_GROUP).toString();
			QString moveToGroup = action->data(ADR_TO_GROUP).toString();
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
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
					roster->moveItemToGroup(contactJid,groupName,moveToGroup);
				}
			}
			else
				roster->moveItemToGroup(contactJid,groupName,moveToGroup);
		}
	}
}

void RosterChanger::onRemoveItemFromGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString contactJid = action->data(ADR_CONTACT_JID).toString();
			QString groupName = action->data(ADR_GROUP).toString();
			roster->removeItemFromGroup(contactJid,groupName);
		}
	}
}

void RosterChanger::onRemoveItemFromRoster(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			Jid contactJid = action->data(ADR_CONTACT_JID).toString();
			if (roster->rosterItem(contactJid).isValid)
			{
				if (QMessageBox::question(NULL,tr("Remove contact"),
					tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(Qt::escape(contactJid.bare())),
					QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				{
					roster->removeItem(contactJid);
				}
			}
			else if (FRostersModel)
			{
				QMultiMap<int, QVariant> findData;
				findData.insert(RDR_TYPE,RIT_CONTACT);
				findData.insert(RDR_TYPE,RIT_AGENT);
				findData.insert(RDR_PREP_BARE_JID,contactJid.pBare());
				IRosterIndex *streamIndex = FRostersModel->streamRoot(streamJid);
				foreach(IRosterIndex *index, streamIndex->findChilds(findData,true))
					FRostersModel->removeRosterIndex(index);

				IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(streamJid,contactJid) : NULL;
				if (window)
					window->closeTabPage();
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
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
		IRoster *toRoster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(toStreamJid) : NULL;
		IRoster *fromRoster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(fromStreamJid) : NULL;
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			if (FRostersView && FRostersView->rostersModel())
			{
				IRosterIndex *sroot = FRostersView->rostersModel()->streamRoot(roster->streamJid());
				IRosterIndex *index = FRostersView->rostersModel()->findGroupIndex(RIT_GROUP,action->data(ADR_GROUP).toString(),roster->groupDelimiter(),sroot);
				if (index)
					FRostersView->editRosterIndex(RDR_NAME,index);
			}
			else
			{
				QString groupDelim = roster->groupDelimiter();
				QString groupName = action->data(ADR_GROUP).toString();
				QStringList groupTree = groupName.split(groupDelim,QString::SkipEmptyParts);
				//QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),QLineEdit::Normal,groupTree.last(),&ok);

				QInputDialog *dialog = new QInputDialog;
				dialog->setProperty("groupTree", groupTree);
				dialog->setProperty("groupName", groupName);
				dialog->setProperty("streamJid", streamJid);
				dialog->setInputMode(QInputDialog::TextInput);
				dialog->setLabelText(tr("<font size=+2>Rename group</font><br>Enter new group name:"));
				dialog->setWindowTitle(tr("Rename group"));
				dialog->setProperty("rename", true);
				dialog->setTextValue(groupTree.last());
				connect(dialog, SIGNAL(textValueSelected(QString)), SLOT(onRenameGroupDialogAccepted(QString)));

				CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
				if (border)
				{
					border->setAttribute(Qt::WA_DeleteOnClose, true);
					border->setMaximizeButtonVisible(false);
					border->setMinimizeButtonVisible(false);
					connect(border, SIGNAL(closeClicked()), dialog, SLOT(reject()));
					connect(dialog, SIGNAL(rejected()), border, SLOT(close()));
					connect(dialog, SIGNAL(accepted()), border, SLOT(close()));
					border->setResizable(false);
					border->show();
				}
				else
				{
					dialog->show();
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupDelim = roster->groupDelimiter();
			QString groupName = action->data(ADR_GROUP).toString();
			QString copyToGroup = action->data(ADR_TO_GROUP).toString();
			if (copyToGroup.endsWith(groupDelim))
			{
				bool ok = false;
				QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"), QLineEdit::Normal,QString(),&ok);
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupName = action->data(ADR_GROUP).toString();
			if (FEmptyGroups.contains(groupName))
			{
				IRosterIndex *group = FRostersModel!=NULL ? FRostersModel->findGroupIndex(RIT_GROUP,groupName,roster->groupDelimiter(),FRostersModel->streamRoot(streamJid)) : NULL;
				if (group)
				{
					group->instance()->deleteLater();
				}
			}
			else
			{
				roster->removeGroup(groupName);
			}
		}
	}
}

void RosterChanger::onRemoveGroupItems(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QString groupName = action->data(ADR_GROUP).toString();
			QList<IRosterItem> ritems = roster->groupItems(groupName);
			if (ritems.count()>0 &&	QMessageBox::question(NULL,tr("Remove contacts"),tr("You are assured that wish to remove %1 contact(s) from roster?").arg(ritems.count()),QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItems(ritems);
			}
		}
	}
}

void RosterChanger::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (AItem.subscription != ABefore.subscription)
	{
		if (AItem.subscription == SUBSCRIPTION_REMOVE)
		{
			if (isSilentSubscribe(ARoster->streamJid(), AItem.itemJid))
				insertAutoSubscribtion(ARoster->streamJid(), AItem.itemJid, true, false, false);
			else
				removeAutoSubscribtion(ARoster->streamJid(), AItem.itemJid);
		}
		else if (AItem.subscription == SUBSCRIPTION_BOTH)
		{
			removeObsoleteChatNotices(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,true);
			removeObsoleteChatNotices(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,false);
		}
		else if (AItem.subscription == SUBSCRIPTION_FROM)
		{
			removeObsoleteChatNotices(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,true);
		}
		else if (AItem.subscription == SUBSCRIPTION_TO)
		{
			removeObsoleteChatNotices(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,false);
		}
	}

	if (AItem.ask != ABefore.ask)
	{
		if (AItem.ask == SUBSCRIPTION_SUBSCRIBE)
		{
			removeObsoleteChatNotices(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribe,true);
		}
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
		else if (FMessageProcessor)
		{
			FMessageProcessor->createMessageWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
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
	int pendingActions = FPendingChatNotices.value(AWindow->streamJid()).value(AWindow->contactJid().bare()).actions;
	bool isService = FMetaContacts!=NULL ? FMetaContacts->metaDescriptorByItem(AWindow->contactJid()).service : false;
	if (!isService && pendingActions==NTA_NO_ACTIONS)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AWindow->streamJid()) : NULL;
		IRosterItem ritem = roster!=NULL ? roster->rosterItem(AWindow->contactJid()) : IRosterItem();
		if (roster && !ritem.isValid)
		{
			if (!AWindow->contactJid().node().isEmpty() && AWindow->streamJid().pBare()!=AWindow->contactJid().pBare())
				insertChatNotice(AWindow,createChatNotice(CNP_SUBSCRIPTION,NTA_ADD_CONTACT|NTA_CLOSE,tr("This contact is not added to your roster."),QString::null));
		}
		else if (roster && ritem.isValid)
		{
			if (ritem.ask!=SUBSCRIPTION_SUBSCRIBE && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
				insertChatNotice(AWindow,createChatNotice(CNP_SUBSCRIPTION,NTA_ASK_SUBSCRIBE|NTA_CLOSE,tr("Request authorization from contact to see his status and mood."),QString::null));
		}
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

void RosterChanger::onViewWidgetCreated(IViewWidget *AWidget)
{
	connect(AWidget->instance(),SIGNAL(viewContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)),
		SLOT(onViewWidgetContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)));
}

void RosterChanger::onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu)
{
	Q_UNUSED(APosition);
	IViewWidget *view = qobject_cast<IViewWidget *>(sender());
	if (view)
	{
		QUrl href = getTextFragmentHref(ASelection);
		QString contact = href.isValid() ? href.path() : ASelection.toPlainText().trimmed();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(view->streamJid()) : NULL;
		if (roster && roster->isOpen() && !roster->rosterItem(contact).isValid)
		{
			IGateServiceDescriptor descriptor = FGateways!=NULL ? FGateways->gateHomeDescriptorsByContact(contact).value(0) : IGateServiceDescriptor();
			if (!descriptor.id.isEmpty() && !(descriptor.needGate && descriptor.readOnly))
			{
				if (!descriptor.needGate || !FGateways->gateDescriptorServices(roster->streamJid(),descriptor).isEmpty())
				{
					Action *action = new Action(AMenu);
					action->setText(tr("Create new contact..."));
					action->setIcon(RSR_STORAGE_MENUICONS,descriptor.iconKey);
					action->setData(ADR_STREAM_JID,roster->streamJid().full());
					action->setData(ADR_CONTACT_TEXT,contact);
					connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
					AMenu->addAction(action,AG_VWCM_ROSTERCHANGER_ADD_CONTACT);
					AMenu->setDefaultAction(action);
				}
			}
		}
	}
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
			if (window->isActiveTabPage())
			{
				removeNotifies(window);
			}
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

Q_EXPORT_PLUGIN2(plg_rosterchanger, RosterChanger)
