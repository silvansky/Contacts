#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <definitions/actiongroups.h>
#include <definitions/chatnoticepriorities.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imetacontacts.h>
#include <utils/action.h>
#include <utils/message.h>
#include <utils/iconstorage.h>
#include "addcontactdialog.h"
#include "addmetacontactdialog.h"
#include "subscriptiondialog.h"

struct AutoSubscription {
	AutoSubscription() {
		silent = false;
		autoSubscribe = false;
		autoUnsubscribe = false;
	}
	bool silent;
	bool autoSubscribe;
	bool autoUnsubscribe;
};

struct PendingChatNotice
{
	PendingChatNotice() {
		notifyId=-1;
		priority=-1;
		actions=0;
	}
	int notifyId;
	int priority;
	int actions;
	QString notify;
	QString text;
};

class GroupMenu :
	public Menu
{
	Q_OBJECT;
public:
	GroupMenu(QWidget* AParent = NULL) : Menu(AParent) { }
	virtual ~GroupMenu() {}
protected:
	void mouseReleaseEvent(QMouseEvent *AEvent);
};

class RosterChanger :
			public QObject,
			public IPlugin,
			public IRosterChanger,
			public IOptionsHolder,
			public IRosterDataHolder,
			public IRostersDragDropHandler,
			public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterChanger IOptionsHolder IRosterDataHolder IRostersDragDropHandler IXmppUriHandler);
public:
	RosterChanger();
	~RosterChanger();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTERCHANGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IRosterChanger
	virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual void insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr);
	virtual void removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
	virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
	virtual IAddMetaItemWidget *newAddMetaItemWidget(const Jid &AStreamJid, const QString &AGateDescriptorId, QWidget *AParent);
	virtual QWidget *showAddContactDialog(const Jid &AStreamJid);
signals:
	void addMetaItemWidgetCreated(IAddMetaItemWidget *AWidget);
	void addContactDialogCreated(IAddContactDialog *ADialog);
	void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	QString subscriptionNotify(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType) const;
	Menu *createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups,
		bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
	SubscriptionDialog *createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage);
	IChatWindow *findChatNoticeWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	IChatNotice createChatNotice(int APriority, int AActions, const QString &ANotify, const QString &AText) const;
	int insertChatNotice(IChatWindow *AWindow, const IChatNotice &ANotice);
	void removeWindowChatNotices(IChatWindow *AWindow);
	void removeObsoleteChatNotices(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent);
	QList<int> findNotifies(const Jid &AStreamJid, const Jid &AContactJid) const;
	QList<Action *> createNotifyActions(int AActions);
	void removeNotifies(IChatWindow *AWindow);
	void removeObsoleteNotifies(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent);
	void showNotifyInChatWindow(IChatWindow *AWindow, const QString &ANotify, const QString &AText) const;
protected slots:
	//Operations on subscription
	void onContactSubscription(bool);
	void onSendSubscription(bool);
	void onSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void onSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	//Operations on items
	void onAddItemToGroup(bool);
	void onRenameItem(bool);
	void onCopyItemToGroup(bool);
	void onMoveItemToGroup(bool);
	void onRemoveItemFromGroup(bool);
	void onRemoveItemFromRoster(bool);
	void onChangeItemGroups(bool AChecked);
	//Operations on group
	void onAddGroupToGroup(bool);
	void onRenameGroup(bool);
	void onCopyGroupToGroup(bool);
	void onMoveGroupToGroup(bool);
	void onRemoveGroup(bool);
	void onRemoveGroupItems(bool);
protected slots:
	void onShowAddContactDialog(bool);
	void onShowAddGroupDialog(bool);
	void onGroupNameAccepted(QString);
	void onShowAddAccountDialog(bool);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterClosed(IRoster *ARoster);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onEmptyGroupChildInserted(IRosterIndex *AIndex);
	void onEmptyGroupIndexDestroyed(IRosterIndex *AIndex);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationActionTriggered(bool);
	void onChatWindowActivated();
	void onChatWindowCreated(IChatWindow *AWindow);
	void onChatWindowDestroyed(IChatWindow *AWindow);
	void onViewWidgetCreated(IViewWidget *AWidget);
	void onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu);
	void onShowPendingChatNotices();
	void onChatNoticeActionTriggered(bool);
	void onChatNoticeRemoved(int ANoticeId);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
private:
	IPluginManager *FPluginManager;
	IRosterPlugin *FRosterPlugin;
	IMetaContacts *FMetaContacts;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	IXmppUriQueries *FXmppUriQueries;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	IMainWindowPlugin *FMainWindowPlugin;
	IAccountManager *FAccountManager;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IGateways *FGateways;
private:
	QList<QString> FEmptyGroups;
	QMap<int, int> FNotifyChatNotice;
	QMap<int, int> FChatNoticeActions;
	QMap<int, IChatWindow *> FChatNoticeWindow;
	QList<IChatWindow *> FPendingChatWindows;
	QMultiMap<Jid, Jid> FSubscriptionRequests;
	QMap<Jid, QMap<Jid, PendingChatNotice> > FPendingChatNotices;
	QMap<Jid, QMap<Jid, AutoSubscription> > FAutoSubscriptions;
};

#endif // ROSTERCHANGER_H
