#ifndef RAMBLERMAILNOTIFY_H
#define RAMBLERMAILNOTIFY_H

#define RAMBLERMAILNOTIFY_UUID "{7EDE7B07-D284-4cd9-AE63-46EFBD4DE683}"

#include <QObject>
#include <definitions/stylesheets.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/namespaces.h>
#include <definitions/notificators.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/notificationdataroles.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterfootertextorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/istatusicons.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessageprocessor.h>
#include <utils/iconstorage.h>

class RamblerMailNotify : 
	public QObject,
	public IPlugin,
	public IStanzaHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaHandler);
public:
	RamblerMailNotify();
	~RamblerMailNotify();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return RAMBLERMAILNOTIFY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
protected:
	IRosterIndex *findMailIndex(const Jid &AStreamJid) const;
	void updateMailIndex(const Jid &AStreamJid);
	void insertMailNotify(const Jid &AStreamJid, const Stanza &AStanza);
protected slots:
	void onStreamAdded(const Jid &AStreamJid);
	void onStreamRemoved(const Jid &AStreamJid);
	void onRosterStateChanged(IRoster *ARoster);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onRosterNotifyActivated(int ANotifyId);
	void onRosterNotifyRemoved(int ANotifyId);
private:
	IGateways *FGateways;
	IRosterPlugin *FRosterPlugin;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IStatusIcons *FStatusIcons;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IMessageProcessor *FMessageProcessor;
private:
	int FAvatarLabelId;
	int FSHIMailNotify;
	QList<IRosterIndex *> FMailIndexes;
	QMap<IRosterIndex *, int> FIndexRosterNotify;
	QMultiMap<IRosterIndex *, int> FIndexPopupNotifies;
};

#endif // RAMBLERMAILNOTIFY_H
