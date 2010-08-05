#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b921f55e-e19b-4567-af26-0d783909c630}"

#include <QTimer>
#include <QVariant>
#include <definations/messagedataroles.h>
#include <definations/messagehandlerorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterlabelorders.h>
#include <definations/rosterclickhookerorders.h>
#include <definations/notificationdataroles.h>
#include <definations/vcardvaluenames.h>
#include <definations/actiongroups.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/soundfiles.h>
#include <definations/optionvalues.h>
#include <definations/optionwidgetorders.h>
#include <definations/toolbargroups.h>
#include <definations/xmppurihandlerorders.h>
#include <definations/tabpagenotifypriorities.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iavatars.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/options.h>
#include "usercontextmenu.h"

struct WindowStatus
{
	QDateTime createTime;
	QDateTime historyTime;
	QString lastStatusShow;
	QList<QDate> separators;
	QList<int> notified;
	QList<Message> unread;
	QList<Message> offline;
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

class ChatMessageHandler :
			public QObject,
			public IPlugin,
			public IMessageHandler,
			public ITabPageHandler,
			public IXmppUriHandler,
			public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler ITabPageHandler IXmppUriHandler IRostersClickHooker);
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
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool showMessage(int AMessageId);
	virtual bool receiveMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
signals:
	//ITabPageHandler
	void tabPageCreated(ITabPage *ATabPage);
	void tabPageDestroyed(ITabPage *ATabPage);
protected:
	IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
	void updateWindow(IChatWindow *AWindow);
	void removeMessageNotifications(IChatWindow *AWindow);
	void replaceUnreadMessages(IChatWindow *AWindow);
	void sendOfflineMessages(IChatWindow *AWindow);
	void removeOfflineMessage(IChatWindow *AWindow, const QUuid &AContentId);
	IPresence *findPresence(const Jid &AStreamJid) const;
	IPresenceItem findPresenceItem(IPresence *APresence, const Jid &AContactJid) const;
	void showStaticMessages(IChatWindow *AWindow);
	void showHistoryMessages(IChatWindow *AWindow, bool AShowAll = false);
	void setMessageStyle(IChatWindow *AWindow);
	void fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	QUuid showDateSeparator(IChatWindow *AWindow, const QDate &ADate);
	QUuid showStyledStatus(IChatWindow *AWindow, const QString &AMessage);
	QUuid showStyledMessage(IChatWindow *AWindow, const Message &AMessage, const StyleExtension &AExtension = StyleExtension());
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
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
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex * AIndex, int ALabelId, QMultiMap<int,QString> & AToolTips, ToolBarChanger * AToolBarChanger);
	void onPresenceAdded(IPresence *APresence);
	void onPresenceOpened(IPresence *APresence);
	void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
	void onPresenceRemoved(IPresence *APresence);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
	void onNotificationTest(const QString &ANotificatorId, uchar AKinds);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IPresencePlugin *FPresencePlugin;
	IMessageArchiver *FMessageArchiver;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IAvatars *FAvatars;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IXmppUriQueries *FXmppUriQueries;
	INotifications *FNotifications;
private:
	QList<IPresence *> FPrecences;
	QHash<QString, TabPageInfo> FTabPages;
private:
	QList<IChatWindow *> FWindows;
	QMap<IChatWindow *, QTimer *> FWindowTimers;
	QMap<IChatWindow *, WindowStatus> FWindowStatus;
};

#endif // CHATMESSAGEHANDLER_H
