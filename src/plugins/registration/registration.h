#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/dataformtypes.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iregistraton.h>
#include <interfaces/idataforms.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#include <utils/log.h>
#include <utils/stanza.h>
#include <utils/options.h>

class Registration :
	public QObject,
	public IPlugin,
	public IRegistration,
	public IStanzaRequestOwner,
	public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRegistration IStanzaRequestOwner IDataLocalizer);
public:
	Registration();
	~Registration();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return REGISTRATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//IRegistration
	virtual QString sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword);
	virtual QString sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit);
signals:
	//IRegistration
	void registerFields(const QString &AId, const IRegisterFields &AFields);
	void registerSuccess(const QString &AId);
	void registerError(const QString &AId, const QString &ACondition, const QString &AMessage);
protected:
	void registerDiscoFeatures();
private:
	IDataForms *FDataForms;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
private:
	QList<QString> FSendRequests;
	QList<QString> FSubmitRequests;
};

#endif // REGISTRATION_H
