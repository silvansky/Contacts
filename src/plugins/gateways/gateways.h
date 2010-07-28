#ifndef GATEWAYS_H
#define GATEWAYS_H

#include <QSet>
#include <QTimer>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/rosterindextyperole.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/vcardvaluenames.h>
#include <definations/discoitemdataroles.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
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
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include "addlegacyaccountdialog.h"
#include "addlegacycontactdialog.h"
#include "addlegacyaccountoptions.h"
#include "managelegacyaccountsoptions.h"

class Gateways :
			public QObject,
			public IPlugin,
			public IGateways,
			public IOptionsHolder,
			public IStanzaRequestOwner,
			public IDiscoFeatureHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IGateways IOptionsHolder IStanzaRequestOwner IDiscoFeatureHandler);
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
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IGateways
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn);
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	virtual QList<Jid> availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const;
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IPresenceItem servicePresence(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IGateServiceLabel serviceLabel(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual IGateServiceLogin serviceLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const;
	virtual IRegisterSubmit serviceSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateServiceLogin &ALogin) const;
	virtual bool isServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual bool setServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe);
	virtual bool removeService(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID);
	virtual QDialog *showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	virtual QDialog *showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
	void availServicesChanged(const Jid &AStreamJid);
	void streamServicesChanged(const Jid &AStreamJid);
	void serviceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void servicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
	void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt);
	void userJidReceived(const QString &AId, const Jid &AUserJid);
	void errorReceived(const QString &AId, const QString &AError);
protected:
	void registerDiscoFeatures();
	void savePrivateStorageSubscribe(const Jid &AStreamJid);
	IGateServiceDescriptor findGateDescriptor(const IDiscoInfo &AInfo) const;
protected slots:
	void onAddLegacyUserActionTriggered(bool);
	void onLogActionTriggered(bool);
	void onResolveActionTriggered(bool);
	void onKeepActionTriggered(bool);
	void onChangeActionTriggered(bool);
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onRosterOpened(IRoster *ARoster);
	void onRosterItemChanged(IRoster *ARoster, const IRosterItem &AItem);
	void onRosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem);
	void onPrivateStorateOpened(const Jid &AStreamJid);
	void onPrivateStorageLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorateAboutToClose(const Jid &AStreamJid);
	void onPrivateStorateClosed(const Jid &AStreamJid);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onKeepTimerTimeout();
	void onVCardReceived(const Jid &AContactJid);
	void onVCardError(const Jid &AContactJid, const QString &AError);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onDiscoItemsReceived(const IDiscoItems &AItems);
	void onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow);
	void onDiscoItemContextMenu(const QModelIndex AIndex, Menu *AMenu);
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterError(const QString &AId, const QString &AError);
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
private:
	QTimer FKeepTimer;
	QMap<Jid, QSet<Jid> > FKeepConnections;
private:
	QList<QString> FPromptRequests;
	QList<QString> FUserJidRequests;
	QMultiMap<Jid, Jid> FResolveNicks;
	QMultiMap<Jid, Jid> FSubscribeServices;
	QMap<QString, Jid> FShowRegisterRequests;
private:
	Jid FOptionsStreamJid;
	QMap<Jid, IDiscoItems> FStreamDiscoItems;
	QList<IGateServiceDescriptor> FGateDescriptors;
};

#endif // GATEWAYS_H
