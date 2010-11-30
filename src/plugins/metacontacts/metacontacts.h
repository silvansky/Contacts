#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QObject>
#include <QObjectCleanupHandler>
#include <definitions/rosterproxyorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/irostersview.h>
#include "metaroster.h"
#include "metaproxymodel.h"

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts);
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMetaContacts
	virtual IMetaRoster *newMetaRoster(IRoster *ARoster);
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const;
	virtual void removeMetaRoster(IRoster *ARoster);
signals:
	void metaRosterAdded(IMetaRoster *AMetaRoster);
	void metaRosterOpened(IMetaRoster *AMetaRoster);
	void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaRosterClosed(IMetaRoster *AMetaRoster);
	void metaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void metaRosterRemoved(IMetaRoster *AMetaRoster);
protected slots:
	void onMetaRosterOpened();
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaRosterClosed();
	void onMetaRosterEnabled(bool AEnabled);
	void onMetaRosterDestroyed(QObject *AObject);
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
private:
	IRosterPlugin *FRosterPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QList<IMetaRoster *> FMetaRosters;
	QObjectCleanupHandler FCleanupHandler;
};

#endif // METACONTACTS_H
