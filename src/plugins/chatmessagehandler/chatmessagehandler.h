#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b921f55e-e19b-4567-af26-0d783909c630}"

#include <QTimer>
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
	QDateTime startTime;
	QDateTime createTime;
	QDateTime lastMessageTime;
	QString lastStatusShow;
};

class ChatMessageHandler :
			public QObject,
			public IPlugin,
			public IMessageHandler,
			public IXmppUriHandler,
			public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler IRostersClickHooker IXmppUriHandler);
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
protected:
	IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
	void updateWindow(IChatWindow *AWindow);
	void removeActiveMessages(IChatWindow *AWindow);
	void showHistory(IChatWindow *AWindow);
	void setMessageStyle(IChatWindow *AWindow);
	void fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showDateSeparator(IChatWindow *AWindow, const QDateTime &AMessageTime);
	void showStyledStatus(IChatWindow *AWindow, const QString &AMessage);
	void showStyledMessage(IChatWindow *AWindow, const Message &AMessage);
protected slots:
	void onMessageReady();
	void onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
	void onWindowActivated();
	void onWindowClosed();
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
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
private:
	QList<IChatWindow *> FWindows;
	QMultiMap<IChatWindow *,int> FActiveMessages;
	QMap<IViewWidget *, WindowStatus> FWindowStatus;
	QMap<IChatWindow *, QTimer *> FWindowTimers;
};

#endif // CHATMESSAGEHANDLER_H
