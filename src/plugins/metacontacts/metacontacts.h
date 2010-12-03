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
	virtual QString metaRosterFileName(const Jid &AStreamJid) const;
signals:
	void metaRosterAdded(IMetaRoster *AMetaRoster);
	void metaRosterOpened(IMetaRoster *AMetaRoster);
	void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaRosterClosed(IMetaRoster *AMetaRoster);
	void metaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void metaRosterStreamJidAboutToBeChanged(IMetaRoster *AMetaRoster, const Jid &AAfter);
	void metaRosterStreamJidChanged(IMetaRoster *AMetaRoster, const Jid &ABefore);
	void metaRosterRemoved(IMetaRoster *AMetaRoster);
protected slots:
	void onMetaRosterOpened();
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaRosterClosed();
	void onMetaRosterEnabled(bool AEnabled);
	void onMetaRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void onMetaRosterStreamJidChanged(const Jid &ABefour);
	void onMetaRosterDestroyed(QObject *AObject);
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
protected slots:
	void onLoadMetaRosters();
private:
	IRosterPlugin *FRosterPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IRostersViewPlugin *FRostersViewPlugin;
	IPluginManager *FPluginManager;
private:
	QList<IMetaRoster *> FLoadQueue;
	QList<IMetaRoster *> FMetaRosters;
	QObjectCleanupHandler FCleanupHandler;
};

#endif // METACONTACTS_H
