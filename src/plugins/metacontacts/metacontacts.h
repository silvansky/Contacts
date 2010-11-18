#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>

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
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMetaContacts
private:
	IRosterPlugin *FRosterPlugin;
	IStanzaProcessor *FStanzaProcessor;
};

#endif // METACONTACTS_H
