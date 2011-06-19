#ifndef RAMBLERMAILNOTIFY_H
#define RAMBLERMAILNOTIFY_H

#define RAMBLERMAILNOTIFY_UUID "{7EDE7B07-D284-4cd9-AE63-46EFBD4DE683}"

#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/namespaces.h>
#include <definitions/notificators.h>
#include <definitions/gateserviceidentifiers.h>
#include <definitions/metaitemorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/notificationdataroles.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/chatwindowwidgetorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istatusicons.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <utils/iconstorage.h>
#include "mailnotifypage.h"
#include "mailinfowidget.h"
#include "custommailpage.h"

struct MailNotify
{
	Jid streamJid;
	Jid serviceJid;
	Jid contactJid;
	int pageNotifyId;
	int popupNotifyId;
	int rosterNotifyId;
};

class RamblerMailNotify : 
	public QObject,
	public IPlugin,
	public IStanzaHandler,
	public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaHandler IRostersClickHooker);
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
	//IRostersClickHooker
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
protected:
	IRosterIndex *findMailIndex(const Jid &AStreamJid) const;
	IRosterIndex *getMailIndex(const Jid &AStreamJid);
	void updateMailIndex(const Jid &AStreamJid);
	void removeMailIndex(const Jid &AStreamJid);
	MailNotify *findMailNotifyByPopupId(int APopupNotifyId) const;
	MailNotify *findMailNotifyByRosterId(int ARosterNotifyId) const;
	void insertMailNotify(const Jid &AStreamJid, const Stanza &AStanza);
	void removeMailNotify(MailNotify *ANotify);
	void clearMailNotifies(const Jid &AStreamJid);
	void clearMailNotifies(MailNotifyPage *APage);
	MailNotifyPage *findMailNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid) const;
	MailNotifyPage *getMailNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid);
	CustomMailPage *findCustomMailPage(const Jid &AStreamJid, const Jid &AServiceJid) const;
	CustomMailPage *getCustomMailPage(const Jid &AStreamJid, const Jid &AServiceJid);
	void showChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	void showNotifyPage(const Jid &AStreamJid, const Jid &AServiceJid) const;
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onRosterModelStreamRemoved(const Jid &AStreamJid);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationTest(const QString &ANotificatorId, uchar AKinds);
	void onRosterNotifyActivated(int ANotifyId);
	void onRosterNotifyRemoved(int ANotifyId);
	void onChatWindowCreated(IChatWindow *AWindow);
	void onMainNotifyPageShowCustomPage();
	void onMailNotifyPageShowChatWindow(const Jid &AContactJid);
	void onMailNotifyPageActivated();
	void onMailNotifyPageDestroyed();
	void onMetaTabWindowDestroyed();
	void onCustomMailPageShowChatWindow(const Jid &AContactJid);
	void onCustomMailPageDestroyed();
private:
	IGateways *FGateways;
	IXmppStreams *FXmppStreams;
	IRosterPlugin *FRosterPlugin;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IMetaContacts *FMetaContacts;
	IStatusIcons *FStatusIcons;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IServiceDiscovery *FDiscovery;
private:
	int FAvatarLabelId;
	int FSHIMailNotify;
	QList<IRosterIndex *> FMailIndexes;
	QMultiMap<IRosterIndex *, MailNotify *> FMailNotifies;
	QMap<IRosterIndex *, IMetaTabWindow *> FMetaTabWindows;
	QMultiMap<IRosterIndex *, MailNotifyPage *> FNotifyPages;
	QMultiMap<IRosterIndex *, CustomMailPage *> FCustomPages;
};

#endif // RAMBLERMAILNOTIFY_H
