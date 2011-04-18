#include "ramblermailnotify.h"

#define MAIL_INDEX_ID "rambler.mail.notify"

RamblerMailNotify::RamblerMailNotify()
{
	FGateways = NULL;
	FRosterPlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FDiscovery = NULL;
	FStatusIcons = NULL;
}

RamblerMailNotify::~RamblerMailNotify()
{

}

void RamblerMailNotify::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Rambler Mail Notifier");
	APluginInfo->description = tr("Notify of new e-mails in Rambler mail box");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
}

bool RamblerMailNotify::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IGateways").value(0);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterStateChanged(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterStateChanged(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		FRostersView = rostersViewPlugin!=NULL ? rostersViewPlugin->rostersView() : NULL;
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(streamAdded(const Jid &)),SLOT(onStreamAdded(const Jid &)));
			connect(FRostersModel->instance(),SIGNAL(streamRemoved(const Jid &)),SLOT(onStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	return true;
}

void RamblerMailNotify::updateMailIndex(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	if (mindex)
	{
		IRosterIndex *sindex = FRostersModel->streamRoot(AStreamJid);
		int show = sindex!=NULL ? sindex->data(RDR_SHOW).toInt() : IPresence::Offline;
		QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByStatus(show!=IPresence::Offline && show!=IPresence::Error ? IPresence::Online : IPresence::Offline,SUBSCRIPTION_BOTH,false) : QIcon();
		mindex->setData(Qt::DecorationRole, icon);
	}
}

IRosterIndex *RamblerMailNotify::findMailIndex(const Jid &AStreamJid) const
{
	foreach(IRosterIndex *index, FMailIndexes)
		if (index->data(RDR_STREAM_JID).toString() == AStreamJid.pFull())
			return index;
	return NULL;
}

void RamblerMailNotify::onStreamAdded(const Jid &AStreamJid)
{
	IRosterIndex *sroot = FRostersModel->streamRoot(AStreamJid);
	if (sroot)
	{
		IRosterIndex *mindex = FRostersModel->createRosterIndex(RIT_MAILNOTIFY,MAIL_INDEX_ID,sroot);
		mindex->setData(Qt::DisplayRole,tr("Mail"));
		mindex->setData(RDR_TYPE_ORDER,RITO_MAILNOTIFY);
		if (FRostersView)
			FRostersView->insertFooterText(FTO_ROSTERSVIEW_STATUS,tr("No new messages"),mindex);
		FMailIndexes.append(mindex);
		FRostersModel->insertRosterIndex(mindex,sroot);
		updateMailIndex(AStreamJid);
	}
}

void RamblerMailNotify::onStreamRemoved(const Jid &AStreamJid)
{
	IRosterIndex *mindex = findMailIndex(AStreamJid);
	FMailIndexes.removeAll(mindex);
}

void RamblerMailNotify::onRosterStateChanged(IRoster *ARoster)
{
	updateMailIndex(ARoster->streamJid());
}

Q_EXPORT_PLUGIN2(plg_ramblermailnotify, RamblerMailNotify)
