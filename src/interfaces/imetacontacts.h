#ifndef IMETACONTACTS_H
#define IMETACONTACTS_H

#include <QSet>
#include <interfaces/iroster.h>

#define METACONTACTS_UUID "{D2E1D146-F98F-4868-89C0-308F72062BFA}"

struct IMetaContact 
{
	IMetaContact() {
		isValid = false;
	}
	bool isValid;
	Jid metaId;
	QString name;
	QSet<Jid> items;
	QSet<QString> groups;
};

class IMetaRoster 
{
public:
	virtual QObject *instance() =0;
	virtual IRoster *roster() const =0;
};

class IMetaContacts
{
public:
	virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IMetaRoster,"Virtus.Plugin.IMetaRoster/1.0")
Q_DECLARE_INTERFACE(IMetaContacts,"Virtus.Plugin.IMetaContacts/1.0")

#endif
