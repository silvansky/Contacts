#ifndef SMSMESSAGEHANDLER_H
#define SMSMESSAGEHANDLER_H

#include <QTimer>
#include <definitions/namespaces.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/chatwindowwidgetorders.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ismsmessagehandler.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/iramblerhistory.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/irostersview.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/inotifications.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <utils/log.h>
#include <utils/stanza.h>
#include "smsinfowidget.h"

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
	QList<Message> requested;
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
	QString notice;
};

enum HisloryLoadState {
	HLS_READY,
	HLS_WAITING,
	HLS_FINISHED,
	HLS_FAILED
};

class SmsMessageHandler :
	public QObject,
	public IPlugin,
	public ISmsMessageHandler,
	public IMessageHandler,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public ITabPageHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISmsMessageHandler IMessageHandler IStanzaHandler IStanzaRequestOwner ITabPageHandler);
public:
	SmsMessageHandler();
	~SmsMessageHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SMSMESSAGEHANDLER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//ITabPageHandler
	virtual bool tabPageAvail(const QString &ATabPageId) const;
	virtual ITabPage *tabPageFind(const QString &ATabPageId) const;
	virtual ITabPage *tabPageCreate(const QString &ATabPageId);
	virtual Action *tabPageAction(const QString &ATabPageId, QObject *AParent);
	//IMessageHandler
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool showMessage(int AMessageId);
	virtual bool receiveMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//SmsMessageHandler
	virtual bool isSmsContact(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual int smsBalance(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual bool requestSmsBalance(const Jid &AStreamJid, const Jid &AServiceJid);
signals:
	void smsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance);
	//ITabPageHandler
	void tabPageCreated(ITabPage *ATabPage);
	void tabPageDestroyed(ITabPage *ATabPage);
protected:
	int smsBalanceFromStanza(const Stanza &AStanza) const;
	void setSmsBalance(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance);
protected:
	IRoster *findRoster(const Jid &AStreamJid) const;
	IRosterItem findRosterItem(const Jid &AStreamJid, const Jid &AContactJid) const;
	IPresenceItem findPresenceItem(IPresence *APresence, const Jid &AContactJid) const;
protected:
	IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExactMatch = true) const;
	void clearWindow(IChatWindow *AWindow);
	void updateWindow(IChatWindow *AWindow);
	void resetWindowStatus(IChatWindow *AWindow);
	void removeMessageNotifications(IChatWindow *AWindow);
	void replaceUnreadMessages(IChatWindow *AWindow);
	void replaceRequestedMessage(IChatWindow *AWindow, const QString &AMessageId, bool AReceived);
protected:
	void requestHistoryMessages(IChatWindow *AWindow, int ACount);
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
	void onWindowActivated();
	void onWindowClosed();
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onOpenTabPageAction(bool);
	void onNotReceivedTimerTimeout();
	void onRamblerHistoryMessagesLoaded(const QString &AId, const IRamblerHistoryMessages &AMessages);
	void onRamblerHistoryRequestFailed(const QString &AId, const QString &AError);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onPresenceOpened(IPresence *APresence);
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IMessageStyles *FMessageStyles;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IRamblerHistory *FRamblerHistory;
	IXmppStreams *FXmppStreams;
	IServiceDiscovery *FDiscovery;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IStanzaProcessor *FStanzaProcessor;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	INotifications *FNotifications;
private:
	QList<IRoster *> FRosters;
	QHash<QString, TabPageInfo> FTabPages;
private:
	QList<IChatWindow *> FWindows;
	QMap<IChatWindow *, QTimer *> FWindowTimers;
	QMap<IChatWindow *, WindowStatus> FWindowStatus;
private:
	QMap<QString, IChatWindow *> FHistoryRequests;
private:
	QTimer FNotReceivedTimer;
	QMap<Jid, int> FSHISmsBalance;
	QMap<Jid, int> FSHIMessageReceipts;
	QMap<QString, Jid> FSmsBalanceRequests;
	QMap<Jid, QMap<Jid, int> > FSmsBalance;
};

#endif // SMSMESSAGEHANDLER_H
