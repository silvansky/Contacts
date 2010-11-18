#include "metacontacts.h"

MetaContacts::MetaContacts()
{
	FRosterPlugin = NULL;
	FStanzaProcessor = NULL;
}

MetaContacts::~MetaContacts()
{

}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Meta Contact");
	APluginInfo->description = tr("Allows other modules to get information about meta contacts in roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(ROSTER_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MetaContacts::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	return FRosterPlugin!=NULL && FStanzaProcessor!=NULL;
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
