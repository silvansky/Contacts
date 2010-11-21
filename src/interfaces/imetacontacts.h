#ifndef IMETACONTACTS_H
#define IMETACONTACTS_H

#include <QSet>
#include <interfaces/iroster.h>

#define METACONTACTS_UUID "{D2E1D146-F98F-4868-89C0-308F72062BFA}"

struct IMetaContact 
{
	Jid id;
	QString name;
	QSet<Jid> items;
	QSet<QString> groups;
};

class IMetaRoster 
{
public:
	virtual QObject *instance() =0;
	virtual Jid streamJid() const =0;
	virtual IRoster *roster() const =0;
	virtual bool isOpen() const =0;
	virtual QList<Jid> metaContacts() const =0;
	virtual IMetaContact metaContact(const Jid &AMetaId) const =0;
	virtual IMetaContact itemMetaContact(const Jid &AItemJid) const =0;
	virtual QSet<QString> groups() const =0;
	virtual QList<IMetaContact> groupContacts(const QString &AGroup) const =0;
protected:
	virtual void metaRosterOpened() =0;
	virtual void metaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore) =0;
	virtual void metaRosterClosed() =0;
};

class IMetaContacts
{
public:
	virtual QObject *instance() =0;
	virtual IMetaRoster *newMetaRoster(IRoster *ARoster) =0;
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const =0;
	virtual void removeMetaRoster(IRoster *ARoster) =0;
protected:
	virtual void metaRosterAdded(IMetaRoster *AMetaRoster) =0;
	virtual void metaRosterOpened(IMetaRoster *AMetaRoster) =0;
	virtual void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore) =0;
	virtual void metaRosterClosed(IMetaRoster *AMetaRoster) =0;
	virtual void metaRosterRemoved(IMetaRoster *AMetaRoster) =0;
};

Q_DECLARE_INTERFACE(IMetaRoster,"Virtus.Plugin.IMetaRoster/1.0")
Q_DECLARE_INTERFACE(IMetaContacts,"Virtus.Plugin.IMetaContacts/1.0")

#endif
