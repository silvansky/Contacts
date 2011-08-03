#ifndef GATEWAYS_H
#define GATEWAYS_H

#include <QSet>
#include <QTimer>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/gateserviceidentifiers.h>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/internalnoticepriorities.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/irostersview.h>
#include <interfaces/ivcard.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iregistraton.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/idataforms.h>
#include <interfaces/imainwindow.h>
#include <interfaces/inotifications.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include <utils/options.h>
#include "addlegacyaccountdialog.h"
#include "addlegacyaccountoptions.h"
#include "addfacebookaccountdialog.h"
#include "managelegacyaccountsoptions.h"
#include "legacyaccountfilter.h"

class Gateways :
	public QObject,
	public IPlugin,
	public IGateways,
	public IOptionsHolder,
	public IStanzaRequestOwner
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IGateways IOptionsHolder IStanzaRequestOwner)
public:
	Gateways();
	~Gateways();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return GATEWAYS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IGateways
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn);
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	virtual QList<IGateServiceDescriptor> gateDescriptors() const;
	virtual IGateServiceDescriptor gateDescriptorById(const QString &ADescriptorId) const;
	virtual QList<IGateServiceDescriptor> gateHomeDescriptorsByContact(const QString &AContact) const;
	virtual QList<IGateServiceDescriptor> gateAvailDescriptorsByContact(const QString &AContact) const;
	virtual int gateDescriptorStatus(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor) const;
	virtual QString formattedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const;
	virtual QString normalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact, bool AComplete = false) const;
	virtual QString checkNormalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const;
	virtual QList<Jid> availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const;
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const;
	virtual QList<Jid> gateDescriptorServices(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, bool AStreamOnly = false) const;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IPresenceItem servicePresence(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IGateServiceDescriptor serviceDescriptor(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IGateServiceLogin serviceLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const;
	virtual IRegisterSubmit serviceSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateServiceLogin &ALogin) const;
	virtual bool isServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual bool setServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe);
	virtual bool removeService(const Jid &AStreamJid, const Jid &AServiceJid, bool AWithContacts);
	virtual QString legacyIdFromUserJid(const Jid &AUserJid) const;
	virtual QString sendLoginRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID);
	virtual QDialog *showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
	void availServicesChanged(const Jid &AStreamJid);
	void streamServicesChanged(const Jid &AStreamJid);
	void serviceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void servicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
	void loginReceived(const QString &AId, const QString &ALogin);
	void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt);
	void userJidReceived(const QString &AId, const Jid &AUserJid);
	void errorReceived(const QString &AId, const QString &AError);
protected:
	void registerDiscoFeatures();
	void startAutoLogin(const Jid &AStreamJid);
	IGateServiceDescriptor findGateDescriptor(const IDiscoInfo &AInfo) const;
	void insertConflictNotice(const Jid &AStreamJid, const Jid &AServiceJid, const QString &ALogin);
	void removeConflictNotice(const Jid &AStreamJid, const Jid &AServiceJid);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onRosterOpened(IRoster *ARoster);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPrivateStorateOpened(const Jid &AStreamJid);
	void onPrivateStorageLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorateAboutToClose(const Jid &AStreamJid);
	void onPrivateStorateClosed(const Jid &AStreamJid);
	void onKeepTimerTimeout();
	void onVCardReceived(const Jid &AContactJid);
	void onVCardError(const Jid &AContactJid, const QString &AError);
	void onDiscoInfoChanged(const IDiscoInfo &AInfo);
	void onDiscoItemsReceived(const IDiscoItems &AItems);
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterSuccess(const QString &AId);
	void onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage);
	void onInternalNoticeReady();
	void onInternalAccountNoticeActionTriggered();
	void onInternalConflictNoticeActionTriggered();
	void onInternalNoticeRemoved(int ANoticeId);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IPluginManager *FPluginManager;
	IServiceDiscovery *FDiscovery;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IPrivateStorage *FPrivateStorage;
	IRegistration *FRegistration;
	IRosterChanger *FRosterChanger;
	IRostersViewPlugin *FRostersViewPlugin;
	IVCardPlugin *FVCardPlugin;
	IStatusIcons *FStatusIcons;
	IOptionsManager *FOptionsManager;
	IDataForms *FDataForms;
	IMainWindowPlugin *FMainWindowPlugin;
	INotifications *FNotifications;
private:
	QTimer FKeepTimer;
	QMap<Jid, QSet<Jid> > FKeepConnections;
private:
	QList<QString> FPromptRequests;
	QList<QString> FUserJidRequests;
	QMultiMap<Jid, Jid> FResolveNicks;
	QMap<QString, Jid> FLoginRequests;
	QMap<QString, QPair<Jid,Jid> > FAutoLoginRequests;
private:
	int FInternalNoticeId;
	Jid FOptionsStreamJid;
	QMap<Jid, IDiscoItems> FStreamDiscoItems;
	QMultiMap<Jid, Jid> FStreamAutoRegServices;
	QList<IGateServiceDescriptor> FGateDescriptors;
private:
	QMap<int, Jid> FConflictNotifies;
	QMap<QString, Jid> FConflictLoginRequests;
	QMap<Jid, QMap<Jid, int> > FConflictNotices;
};

#endif // GATEWAYS_H
