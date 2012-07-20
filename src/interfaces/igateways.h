#ifndef IGATEWAYS_H
#define IGATEWAYS_H

#include <QDialog>
#include <interfaces/iregistraton.h>
#include <interfaces/ipresence.h>
#include <interfaces/iservicediscovery.h>
#include <utils/jid.h>

#define GATEWAYS_UUID             "{2a3ce0cd-bf67-4f15-8907-b7d0706be4b4}"

#define GATE_PREFIX_PATTERN       "(^gw\\d+\\.|^)%1\\..*"

enum IGateServiceRestrictions
{
	GSR_ADD_CONTACT    = 0x01,
	GSR_DELETE_CONTACT = 0x02,
	GSR_RENAME_CONTACT = 0x04,
	GSR_CHANGE_GROUPS  = 0x08
};

struct IGateServiceLogin
{
	IGateServiceLogin() { 
		isValid = false; 
	}
	bool isValid;
	QString login;
	QString domain;
	QString password;
	QString domainSeparator;
	IRegisterFields fields;
};

struct IGateServiceLabel
{
	QString id;
	QString name;
	QString iconKey;
	QString loginLabel;
	QList<QString> domains;
};

struct IGateServiceDescriptor : 
	public IGateServiceLabel
{
	IGateServiceDescriptor() { 
		needGate = false;
		needLogin = true;
		autoLogin = false;
	}
	bool needGate;
	bool needLogin;
	bool autoLogin;
	QString type;
	QString prefix;
	QString loginField;
	QString domainField;
	QString passwordField;
	QString domainSeparator;
	QString homeContactPattern;
	QString availContactPattern;
	QList<QString> linkedDescriptors;
	QList<QString> blockedDescriptors;
	QMap<QString, QVariant> extraFields;
};

class IGateways
{
public:
	enum GateDescriptorStatuses {
		GDS_UNAVAILABLE,
		GDS_UNREGISTERED,
		GDS_DISABLED,
		GDS_ENABLED
	};
public:
	virtual QObject *instance() =0;
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn) =0;
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const =0;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual QList<IGateServiceDescriptor> gateDescriptors() const =0;
	virtual IGateServiceDescriptor gateDescriptorById(const QString &ADescriptorId) const =0;
	virtual QList<IGateServiceDescriptor> gateHomeDescriptorsByContact(const QString &AContact) const =0;
	virtual QList<IGateServiceDescriptor> gateAvailDescriptorsByContact(const QString &AContact) const =0;
	virtual int gateDescriptorStatus(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor) const =0;
	virtual Jid gateDescriptorRegistrator(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, bool AFree = true) const =0;
	virtual QString formattedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const =0;
	virtual QString normalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact, bool AComplete = false) const =0;
	virtual QString checkNormalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const =0;
	virtual QList<Jid> availRegistrators(const Jid &AStreamJid, bool AFree = true) const =0;
	virtual QList<Jid> availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual quint32 gateDescriptorRestrictions(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor) const =0;
	virtual QList<Jid> gateDescriptorServices(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, bool AStreamOnly = false) const =0;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IPresenceItem servicePresence(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IGateServiceDescriptor serviceDescriptor(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual quint32 serviceRestrictions(const Jid &AStreamJid, const Jid &AServiceJid, bool ACheckPresence = true) const =0;
	virtual IGateServiceLogin serviceLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const =0;
	virtual IRegisterSubmit serviceSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateServiceLogin &ALogin) const =0;
	virtual bool isServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual bool setServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe) =0;
	virtual QString removeService(const Jid &AStreamJid, const Jid &AServiceJid, bool AWithContacts) =0;
	virtual QString legacyIdFromUserJid(const Jid &AUserJid) const =0;
	virtual QString legacyIdFromUserJid(const Jid &AStreamJid, const Jid &AUserJid) const =0;
	virtual QString sendLoginRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID) =0;
	virtual QDialog *showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
protected:
	virtual void availServicesChanged(const Jid &AStreamJid) =0;
	virtual void streamServicesChanged(const Jid &AStreamJid) =0;
	virtual void serviceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual void servicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem) =0;
	virtual void serviceRemoved(const QString &AId) = 0;
	virtual void loginReceived(const QString &AId, const QString &ALogin) =0;
	virtual void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt) =0;
	virtual void userJidReceived(const QString &AId, const Jid &AUserJid) =0;
	virtual void errorReceived(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IGateways,"Virtus.Plugin.IGateways/1.0")

#endif
