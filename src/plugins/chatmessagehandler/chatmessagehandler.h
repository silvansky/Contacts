#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b921f55e-e19b-4567-af26-0d783909c630}"

#include <QTimer>
#include <QVariant>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/iramblerhistory.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iavatars.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imetacontacts.h>
#include <utils/log.h>
#include <utils/options.h>
#include <utils/errorhandler.h>
#include "usercontextmenu.h"

struct WindowStatus
{
	QDateTime createTime;
	QString historyId;
	QDateTime historyTime;
	QUuid historyContentId;
	QString lastStatusShow;
	QList<QDate> separators;
	QList<int> notified;
	QList<Message> unread;
	QList<Message> offline;
	QList<Message> pending;
};

struct TabPageInfo
{
	Jid streamJid;
	Jid contactJid;
	ITabPage *page;
};

struct StyleExtension
{
	StyleExtension() {
		action = IMessageContentOptions::InsertAfter;
		extensions = 0;
	}
	int action;
	int extensions;
	QString contentId;
};

enum HisloryLoadState {
	HLS_READY,
	HLS_WAITING,
	HLS_FINISHED,
	HLS_FAILED
};

class ChatMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler,
	public ITabPageHandler,
	public IXmppUriHandler,
	public IRostersClickHooker
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageHandler ITabPageHandler IXmppUriHandler IRostersClickHooker)
public:
	ChatMessageHandler();
	~ChatMessageHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CHATMESSAGEHANDLER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//ITabPageHandler
	virtual bool tabPageAvail(const QString &ATabPageId) const;
	virtual ITabPage *tabPageFind(const QString &ATabPageId) const;
	virtual ITabPage *tabPageCreate(const QString &ATabPageId);
	virtual Action *tabPageAction(const QString &ATabPageId, QObject *AParent);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IRostersClickHooker
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
signals:
	//ITabPageHandler
	void tabPageCreated(ITabPage *ATabPage);
	void tabPageDestroyed(ITabPage *ATabPage);
protected:
	IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExactMatch = true) const;
	IChatWindow *findNotifiedMessageWindow(int AMessageId) const;
	void clearWindow(IChatWindow *AWindow);
	void updateWindow(IChatWindow *AWindow);
	void resetWindowStatus(IChatWindow *AWindow);
	void removeMessageNotifications(IChatWindow *AWindow);
	void replaceUnreadMessages(IChatWindow *AWindow);
	void sendOfflineMessages(IChatWindow *AWindow);
	void removeOfflineMessage(IChatWindow *AWindow, const QUuid &AContentId);
	void requestHistoryMessages(IChatWindow *AWindow, int ACount);
	IPresence *findPresence(const Jid &AStreamJid) const;
	IPresenceItem findPresenceItem(IPresence *APresence, const Jid &AContactJid) const;
	void showHistoryLinks(IChatWindow *AWindow, HisloryLoadState AState);
protected:
	void setMessageStyle(IChatWindow *AWindow);
	void fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	QUuid showDateSeparator(IChatWindow *AWindow, const QDate &ADate);
	QUuid showStyledStatus(IChatWindow *AWindow, const QString &AMessage);
	QUuid showStyledMessage(IChatWindow *AWindow, const Message &AMessage, const StyleExtension &AExtension = StyleExtension());
protected:
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onMessageReady();
	void onUrlClicked(const QUrl &AUrl);
	void onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
	void onWindowActivated();
	void onWindowClosed();
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onOpenTabPageAction(bool);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger);
	void onPresenceAdded(IPresence *APresence);
	void onPresenceOpened(IPresence *APresence);
	void onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceRemoved(IPresence *APresence);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
	void onNotificationTest(const QString &ATypeId, ushort AKinds);
	void onRamblerHistoryMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages);
	void onRamblerHistoryRequestFailed(const QString &AId, const QString &AError);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRamblerHistory *FRamblerHistory;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IAvatars *FAvatars;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IXmppUriQueries *FXmppUriQueries;
	INotifications *FNotifications;
	IMetaContacts *FMetaContacts;
private:
	QList<IPresence *> FPrecences;
	QHash<QString, TabPageInfo> FTabPages;
private:
	QList<IChatWindow *> FWindows;
	QMap<IChatWindow *, QTimer *> FDestroyTimers;
	QMap<IChatWindow *, WindowStatus> FWindowStatus;
private:
	QMap<QString, IChatWindow *> FHistoryRequests;
};

#endif // CHATMESSAGEHANDLER_H
