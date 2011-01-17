#include "metacontacts.h"

#include <QDir>
#include <QMessageBox>
#include <QInputDialog>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_META_ID         Action::DR_Parametr1
#define ADR_NAME            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FRostersViewPlugin = NULL;
	FStanzaProcessor = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
}

MetaContacts::~MetaContacts()
{
	FCleanupHandler.clear();
}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Meta Contacts");
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
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
		}
	}

	return FRosterPlugin!=NULL && FStanzaProcessor!=NULL;
}

bool MetaContacts::initObjects()
{
	if (FRostersViewPlugin)
	{
		MetaProxyModel *proxyModel = new MetaProxyModel(this, FRostersViewPlugin->rostersView());
		FRostersViewPlugin->rostersView()->insertProxyModel(proxyModel, RPO_METACONTACTS_MODIFIER);
		FRostersViewPlugin->rostersView()->insertClickHooker(RCHO_DEFAULT,this);
	}
	return true;
}

bool MetaContacts::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	Q_UNUSED(AOrder);
	if (AIndex->type() == RIT_METACONTACT)
	{
		IMetaRoster *mroster = findMetaRoster(AIndex->data(RDR_STREAM_JID).toString());
		if (FMessageWidgets && mroster && mroster->isEnabled())
		{
			Jid metaId = AIndex->data(RDR_INDEX_ID).toString();
			IMetaTabWindow *window = newMetaTabWindow(mroster->streamJid(), metaId);
			window->showTabPage();
		}
	}
	return false;
}

IMetaRoster *MetaContacts::newMetaRoster(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster == NULL)
	{
		mroster = new MetaRoster(ARoster,FStanzaProcessor);
		connect(mroster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onMetaRosterDestroyed(QObject *)));
		FCleanupHandler.add(mroster->instance());
		FMetaRosters.append(mroster);
	}
	return mroster;
}

IMetaRoster *MetaContacts::findMetaRoster(const Jid &AStreamJid) const
{
	foreach(IMetaRoster *mroster, FMetaRosters)
		if (mroster->streamJid() == AStreamJid)
			return mroster;
	return NULL;
}

void MetaContacts::removeMetaRoster(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster)
	{
		disconnect(mroster->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onMetaRosterDestroyed(QObject *)));
		FMetaRosters.removeAll(mroster);
		delete mroster->instance();
	}
}

QString MetaContacts::metaRosterFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists("metarosters"))
		dir.mkdir("metarosters");
	dir.cd("metarosters");
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

QList<IMetaTabWindow *> MetaContacts::metaTabWindows() const
{
	return FMetaTabWindows;
}

IMetaTabWindow *MetaContacts::newMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId)
{
	IMetaTabWindow *window = findMetaTabWindow(AStreamJid,AMetaId);
	if (!window)
	{
		IMetaRoster *mroster = findMetaRoster(AStreamJid);
		if (mroster && mroster->isEnabled() && mroster->metaContact(AMetaId).id.isValid())
		{
			window = new MetaTabWindow(FMessageWidgets,mroster,AMetaId);
			connect(window->instance(),SIGNAL(itemPageRequested(const Jid &)),SLOT(onMetaTabWindowItemPageRequested(const Jid &)));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMetaTabWindowDestroyed()));
			FCleanupHandler.add(window->instance());
			FMetaTabWindows.append(window);
			emit metaTabWindowCreated(window);
		}
	}
	return window;
}

IMetaTabWindow *MetaContacts::findMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId) const
{
	foreach(IMetaTabWindow *window, FMetaTabWindows)
	{
		if (window->metaId()==AMetaId && window->metaRoster()->streamJid()==AStreamJid)
			return window;
	}
	return NULL;
}

void MetaContacts::deleteMetaRosterWindows(IMetaRoster *AMetaRoster)
{
	QList<IMetaTabWindow *> windows = FMetaTabWindows;
	foreach(IMetaTabWindow *window, windows)
		if (window->metaRoster() == AMetaRoster)
			delete window->instance();
}

void MetaContacts::onMetaRosterOpened()
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterOpened(mroster);
}

void MetaContacts::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaContactReceived(mroster,AContact,ABefore);
}

void MetaContacts::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaActionResult(mroster,AActionId,AErrCond,AErrMessage);
}

void MetaContacts::onMetaRosterClosed()
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterClosed(mroster);
}

void MetaContacts::onMetaRosterEnabled(bool AEnabled)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterEnabled(mroster,AEnabled);
}

void MetaContacts::onMetaRosterStreamJidAboutToBeChanged(const Jid &AAfter)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
	{
		if (!(mroster->streamJid() && AAfter))
			mroster->saveMetaContacts(metaRosterFileName(mroster->streamJid()));
		emit metaRosterStreamJidAboutToBeChanged(mroster, AAfter);
	}
}

void MetaContacts::onMetaRosterStreamJidChanged(const Jid &ABefore)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
	{
		emit metaRosterStreamJidChanged(mroster, ABefore);
		if (!(mroster->streamJid() && ABefore))
			mroster->loadMetaContacts(metaRosterFileName(mroster->streamJid()));
	}
}

void MetaContacts::onMetaRosterDestroyed(QObject *AObject)
{
	MetaRoster *mroster = static_cast<MetaRoster *>(AObject);
	FMetaRosters.removeAll(mroster);
}

void MetaContacts::onRosterAdded(IRoster *ARoster)
{
	IMetaRoster *mroster = newMetaRoster(ARoster);
	connect(mroster->instance(),SIGNAL(metaRosterOpened()),SLOT(onMetaRosterOpened()));
	connect(mroster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(mroster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
		SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));
	connect(mroster->instance(),SIGNAL(metaRosterClosed()),SLOT(onMetaRosterClosed()));
	connect(mroster->instance(),SIGNAL(metaRosterEnabled(bool)),SLOT(onMetaRosterEnabled(bool)));
	connect(mroster->instance(),SIGNAL(metaRosterStreamJidAboutToBeChanged(const Jid &)),SLOT(onMetaRosterStreamJidAboutToBeChanged(const Jid &)));
	connect(mroster->instance(),SIGNAL(metaRosterStreamJidChanged(const Jid &)),SLOT(onMetaRosterStreamJidChanged(const Jid &)));
	emit metaRosterAdded(mroster);

	FLoadQueue.append(mroster);
	QTimer::singleShot(0,this,SLOT(onLoadMetaRosters()));
}

void MetaContacts::onRosterRemoved(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster)
	{
		deleteMetaRosterWindows(mroster);
		mroster->saveMetaContacts(metaRosterFileName(mroster->streamJid()));
		emit metaRosterRemoved(mroster);
		removeMetaRoster(ARoster);
	}
}

void MetaContacts::onMetaTabWindowItemPageRequested(const Jid &AItemJid)
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	if (window)
	{
		FMessageProcessor->createWindow(window->metaRoster()->streamJid(),AItemJid,Message::Chat,IMessageHandler::SM_ADD_TAB);
	}
}

void MetaContacts::onMetaTabWindowDestroyed()
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	if (window)
	{
		FMetaTabWindows.removeAll(window);
		emit metaTabWindowDestroyed(window);
	}
}

void MetaContacts::onLoadMetaRosters()
{
	foreach(IMetaRoster *mroster, FLoadQueue)
		mroster->loadMetaContacts(metaRosterFileName(mroster->streamJid()));
	FLoadQueue.clear();
}

void MetaContacts::onChatWindowCreated(IChatWindow *AWindow)
{
	IMetaRoster *mroster = findMetaRoster(AWindow->streamJid());
	if (mroster && mroster->isEnabled())
	{
		Jid metaId = mroster->itemMetaContact(AWindow->contactJid());
		if (metaId.isValid())
		{
			IMetaTabWindow *window = newMetaTabWindow(mroster->streamJid(), metaId);
			window->setItemPage(AWindow->contactJid(),AWindow);
		}
	}
}

void MetaContacts::onRenameContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			Jid metaId = action->data(ADR_META_ID).toString();
			QString oldName = action->data(ADR_NAME).toString();
			bool ok = false;
			QString newName = QInputDialog::getText(NULL,tr("Contact name"),tr("Enter name for contact"), QLineEdit::Normal, oldName, &ok);
			if (ok && !newName.isEmpty() && newName != oldName)
				mroster->renameContact(metaId, newName);
		}
	}
}

void MetaContacts::onDeleteContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			Jid metaId = action->data(ADR_META_ID).toString();
			IMetaContact contact = mroster->metaContact(metaId);
			if (QMessageBox::question(NULL,tr("Remove contact"),
				tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(!contact.name.isEmpty() ? contact.name : contact.id.node()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				mroster->deleteContact(metaId);
			}
		}
	}
}

void MetaContacts::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IMetaRoster *mroster = findMetaRoster(streamJid);
	if (mroster && mroster->isOpen())
	{
		int itemType = AIndex->data(RDR_TYPE).toInt();
		if (itemType == RIT_METACONTACT)
		{
			Jid metaId = AIndex->data(RDR_INDEX_ID).toString();
			const IMetaContact &contact = mroster->metaContact(metaId);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid.full());
			data.insert(ADR_META_ID,metaId.full());
			data.insert(ADR_NAME, contact.name);

			Action *action = new Action(AMenu);
			action->setText(tr("Rename..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRenameContact(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);

			action = new Action(AMenu);
			action->setText(tr("Delete"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onDeleteContact(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
