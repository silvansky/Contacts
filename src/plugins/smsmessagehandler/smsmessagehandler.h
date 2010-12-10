#ifndef SMSMESSAGEHANDLER_H
#define SMSMESSAGEHANDLER_H

#define SMSMESSAGEHANDLER_UUID "{7A7DBF1A-4C1C-4ba5-9A82-ACD7A204A438}"

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>

class SmsMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageHandler);// ITabPageHandler IXmppUriHandler IRostersClickHooker);

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
	virtual bool startPlugin() { return true; }
	//IMessageHandler
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool showMessage(int AMessageId);
	virtual bool receiveMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool createWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);

protected:
	IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);

private:
	QList<IChatWindow *> FWindows;
};

#endif // SMSMESSAGEHANDLER_H
