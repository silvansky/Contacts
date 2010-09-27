#ifndef IGATEWAYS_H
#define IGATEWAYS_H

#include <QDialog>
#include <interfaces/iregistraton.h>
#include <interfaces/ipresence.h>
#include <interfaces/iservicediscovery.h>
#include <utils/jid.h>

#define GATEWAYS_UUID "{2a3ce0cd-bf67-4f15-8907-b7d0706be4b4}"

struct IGateServiceLabel
{
	IGateServiceLabel() { 
		valid = false; 
	}
	bool valid;
	QString name;
	QString iconKey;
	QString loginLabel;
	QList<QString> domains;
};

struct IGateServiceLogin
{
	IGateServiceLogin() { 
		valid = false; 
	}
	bool valid;
	QString login;
	QString domain;
	QString password;
	IRegisterFields fields;
};

struct IGateServiceDescriptor : public IGateServiceLabel
{
	IGateServiceDescriptor() { 
		valid = false;
		gateRequired = true;
	}
	bool gateRequired;
	QString type;
	QString prefix;
	QString loginField;
	QString domainField;
	QString passwordField;
	QString domainSeparator;
	QString homeContactRegexp;
	QString availContactRegexp;
	QMap<QString, QVariant> extraFields;
};

class IGateways
{
public:
	virtual QObject *instance() =0;
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn) =0;
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const =0;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual QList<QString> availDescriptors() const =0;
	virtual IGateServiceDescriptor descriptorByName(const QString &AServiceName) const =0;
	virtual IGateServiceDescriptor descriptorByContact(const QString &AContact) const =0;
	virtual QList<IGateServiceDescriptor> descriptorsByContact(const QString &AContact) const =0;
	virtual QList<Jid> availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IPresenceItem servicePresence(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IGateServiceLabel serviceLabel(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IGateServiceLogin serviceLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const =0;
	virtual IRegisterSubmit serviceSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateServiceLogin &ALogin) const =0;
	virtual bool isServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual bool setServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe) =0;
	virtual bool removeService(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendLoginRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID) =0;
	virtual QDialog *showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
	virtual QDialog *showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
protected:
	virtual void availServicesChanged(const Jid &AStreamJid) =0;
	virtual void streamServicesChanged(const Jid &AStreamJid) =0;
	virtual void serviceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual void servicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem) =0;
	virtual void loginReceived(const QString &AId, const QString &ALogin) =0;
	virtual void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt) =0;
	virtual void userJidReceived(const QString &AId, const Jid &AUserJid) =0;
	virtual void errorReceived(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IGateways,"Virtus.Plugin.IGateways/1.0")

#endif
