#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <definations/actiongroups.h>
#include <definations/noticepriorities.h>
#include <definations/rosterlabelorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/multiuserdataroles.h>
#include <definations/notificators.h>
#include <definations/notificationdataroles.h>
#include <definations/rosterdragdropmimetypes.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/soundfiles.h>
#include <definations/xmppurihandlerorders.h>
#include <definations/tabpagenotifypriorities.h>
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
#include <utils/action.h>
#include "addcontactdialog.h"
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

struct PendingNotice
{
	PendingNotice() {
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

class RosterChanger :
			public QObject,
			public IPlugin,
			public IRosterChanger,
			public IOptionsHolder,
			public IRostersDragDropHandler,
			public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterChanger IOptionsHolder IRostersDragDropHandler IXmppUriHandler);
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
	virtual IAddContactDialog *showAddContactDialog(const Jid &AStreamJid);
signals:
	void addContactDialogCreated(IAddContactDialog *ADialog);
	void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
protected:
	QString subscriptionNotify(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType) const;
	Menu *createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups,
		bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
	SubscriptionDialog *createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage);
	IChatWindow *findNoticeWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	INotice createNotice(int APriority, int AActions, const QString &ANotify, const QString &AText) const;
	int insertNotice(IChatWindow *AWindow, const INotice &ANotice);
	void removeObsoleteNotices(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent);
	QList<int> findNotifies(const Jid &AStreamJid, const Jid &AContactJid) const;
	QList<Action *> createNotifyActions(int AActions);
	void showNotifyInChatWindow(IChatWindow *AWindow, const QString &ANotify, const QString &AText) const;
	void removeChatWindowNotifies(IChatWindow *AWindow);
	void removeObsoleteNotifies(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent);
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
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterClosed(IRoster *ARoster);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationActionTriggered(bool);
	void onChatWindowActivated();
	void onChatWindowCreated(IChatWindow *AWindow);
	void onChatWindowDestroyed(IChatWindow *AWindow);
	void onShowPendingNotices();
	void onNoticeActionTriggered(bool);
	void onNoticeRemoved(int ANoticeId);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
private:
	IPluginManager *FPluginManager;
	IRosterPlugin *FRosterPlugin;
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
private:
	QMap<int, int> FNotifyNotice;
	QMap<int, int> FNoticeActions;
	QMap<int, IChatWindow *> FNoticeWindow;
	QList<IChatWindow *> FPendingChatWindows;
	QMap<Jid, QMap<Jid, PendingNotice> > FPendingNotices;
	QMap<Jid, QMap<Jid, AutoSubscription> > FAutoSubscriptions;
};

#endif // ROSTERCHANGER_H
