#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define MASSSENDHANDLER_UUID "{d803d748-c95a-4631-bd04-952cf8e1031c}"

#include <QMultiMap>
#include <definations/messagehandlerorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterlabelorders.h>
#include <definations/notificationdataroles.h>
#include <definations/optionwidgetorders.h>
#include <definations/actiongroups.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/soundfiles.h>
#include <definations/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iaccountmanager.h>
#include <utils/errorhandler.h>

class MassSendHandler :
			public QObject,
			public IPlugin,
			public IMessageHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler);
public:
	MassSendHandler();
	~MassSendHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MASSSENDHANDLER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMessageHandler
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool showMessage(int AMessageId);
	virtual bool receiveMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
protected:
	IMassSendDialog *getDialog(const Jid &AStreamJid);
	IMassSendDialog *findDialog(const Jid &AStreamJid);
	void showDialog(IMassSendDialog *ADialog);
	void updateDialog(IMassSendDialog *ADialog);
	void setMessageStyle(IMassSendDialog *ADialog);
	void fillContentOptions(IMassSendDialog *ADialog, IMessageContentOptions &AOptions) const;
	void showStyledMessage(IMassSendDialog *ADialog, const Message &AMessage);
protected slots:
	void onMessageReady();
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
	void onMassSendAction();
private:
	IMessageWidgets *FMessageWidgets;
	IMassSendDialog * FMassSendDialog;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IStatusIcons *FStatusIcons;
	IPresencePlugin *FPresencePlugin;
	IRostersView *FRostersView;
	IXmppUriQueries *FXmppUriQueries;
	IMainWindowPlugin * FMainWindowPlugin;
	IAccountManager * FAccountManager;
private:
	QList<IMassSendDialog *> FDialogs;
};

#endif // NORMALMESSAGEHANDLER_H
