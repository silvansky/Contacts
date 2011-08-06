#include "rostersviewplugin.h"

#include <QTimer>
#include <QScrollBar>

const QList<int> GroupsWithCounter = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK << RIT_GROUP_NOT_IN_ROSTER;

RostersViewPlugin::RostersViewPlugin()
{
	FRostersModel = NULL;
	FMainWindowPlugin = NULL;
	FOptionsManager = NULL;
	FRosterPlugin = NULL;
	FAccountManager = NULL;

	FShowOfflineAction = NULL;
	FGroupContactsAction = NULL;

	FSortFilterProxyModel = NULL;
	FLastModel = NULL;
	FStartRestoreExpandState = false;

	FViewSavedState.sliderPos = 0;
	FViewSavedState.currentIndex = NULL;

	FRostersView = new RostersView;
	FRostersView->setObjectName("rostersView");
	connect(FRostersView,SIGNAL(viewModelAboutToBeChanged(QAbstractItemModel *)),SLOT(onViewModelAboutToBeChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(viewModelChanged(QAbstractItemModel *)),SLOT(onViewModelChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onViewIndexCollapsed(const QModelIndex &)));
	connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onViewIndexExpanded(const QModelIndex &)));
	connect(FRostersView,SIGNAL(modelAboutToBeSet(IRostersModel *)),SLOT(onRosterModelAboutToBeSet(IRostersModel *)));
	connect(FRostersView,SIGNAL(destroyed(QObject *)), SLOT(onRostersViewDestroyed(QObject *)));
}

RostersViewPlugin::~RostersViewPlugin()
{
	delete FRostersView;
}

void RostersViewPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster View");
	APluginInfo->description = tr("Displays a hierarchical roster's model");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(ROSTERSMODEL_UUID);
}

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidAboutToBeChanged(IRoster *, const Jid &)),
				SLOT(onRosterStreamJidAboutToBeChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(destroyed(const QUuid &)),SLOT(onAccountDestroyed(const QUuid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FRostersModel!=NULL;
}

bool RostersViewPlugin::initObjects()
{
	IRostersLabel counter;
	counter.order = RLO_GROUP_COUNTER;
	counter.label = RDR_GROUP_COUNTER;
	FGroupCounterLabel = FRostersView->registerLabel(counter);

	FSortFilterProxyModel = new SortFilterProxyModel(this, this);
	FSortFilterProxyModel->setSortLocaleAware(true);
	FSortFilterProxyModel->setDynamicSortFilter(true);
	FSortFilterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	FSortFilterProxyModel->sort(0,Qt::AscendingOrder);
	FRostersView->insertProxyModel(FSortFilterProxyModel,RPO_ROSTERSVIEW_SORTFILTER);

	if (FMainWindowPlugin)
	{
		FShowOfflineAction = new Action(this);
		FShowOfflineAction->setCheckable(true);
		FShowOfflineAction->setText(tr("Show Offline"));
		//FShowOfflineAction->setShortcut(tr("Ctrl+O"));
		FShowOfflineAction->setData(Action::DR_SortString,QString("100"));
		connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflinesAction(bool)));
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FShowOfflineAction,AG_MMENU_ROSTERSVIEW_SHOWOFFLINE,true);

		FGroupContactsAction = new Action(this);
		FGroupContactsAction->setCheckable(true);
		FGroupContactsAction->setText(tr("Group Contacts"));
		FGroupContactsAction->setData(Action::DR_SortString,QString("200"));
		connect(FGroupContactsAction,SIGNAL(triggered(bool)),SLOT(onGroupContactsAction(bool)));
		//FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FGroupContactsAction,AG_MMENU_ROSTERSVIEW_GROUPCONTACTS,true);

		FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);
	}

	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
		FRostersView->setRostersModel(FRostersModel);
	}

	return true;
}

bool RostersViewPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_SHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_SHOWSTATUSTEXT,true);
	Options::setDefaultValue(OPV_ROSTER_SORTBYSTATUS,false);
	Options::setDefaultValue(OPV_ROSTER_GROUPCONTACTS,true);

	if (FOptionsManager)
	{
		FOptionsManager->insertServerOption(OPV_ROSTER_SHOWOFFLINE);
		FOptionsManager->insertServerOption(OPV_ROSTER_SHOWSTATUSTEXT);
		FOptionsManager->insertServerOption(OPV_ROSTER_SORTBYSTATUS);
		FOptionsManager->insertServerOption(OPV_ROSTER_GROUPCONTACTS);

		IOptionsDialogNode dnode = { ONO_ROSTER, OPN_ROSTER, tr("Contact List"),MNI_ROSTERVIEW_OPTIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> RostersViewPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_VIEW, FOptionsManager->optionsHeaderWidget(QString::null,tr("Contact List"),AParent));
		widgets.insertMulti(OWO_ROSTER_VIEW, FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SHOWOFFLINE),tr("Show offline contacts"),AParent));
		//widgets.insertMulti(OWO_ROSTER_VIEW, FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_GROUPCONTACTS),tr("Group contacts"),AParent));

		widgets.insertMulti(OWO_ROSTER_CONTACTS_ORDER, FOptionsManager->optionsHeaderWidget(QString::null,tr("Contacts Order"),AParent));
		widgets.insertMulti(OWO_ROSTER_CONTACTS_ORDER, new RosterContactOrderOptions(AParent));

		widgets.insertMulti(OWO_ROSTER_CONTACTS_VIEW, FOptionsManager->optionsHeaderWidget(QString::null,tr("Contacts View"),AParent));
		widgets.insertMulti(OWO_ROSTER_CONTACTS_VIEW, new RosterContactViewOptions(AParent));
	}
	return widgets;
}

int RostersViewPlugin::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> RostersViewPlugin::rosterDataRoles() const
{
	static QList<int> dataRoles  = QList<int>()
		<< Qt::DisplayRole
		<< Qt::BackgroundColorRole
		<< Qt::ForegroundRole
		<< RDR_FONT_WEIGHT
		<< RDR_FONT_SIZE;
		//<< RDR_GROUP_COUNTER;
	return dataRoles;
}

QList<int> RostersViewPlugin::rosterDataTypes() const
{
	static QList<int> indexTypes  = QList<int>()
		<< RIT_STREAM_ROOT
		<< RIT_GROUP
		<< RIT_GROUP_BLANK
		<< RIT_GROUP_AGENTS
		<< RIT_GROUP_MY_RESOURCES
		<< RIT_GROUP_NOT_IN_ROSTER
		<< RIT_CONTACT
		<< RIT_AGENT
		<< RIT_MY_RESOURCE;
	return indexTypes;
}

QVariant RostersViewPlugin::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	switch (AIndex->type())
	{
	case RIT_STREAM_ROOT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
					display = AIndex->data(RDR_FULL_JID).toString();
				return display;
			}
		case Qt::ForegroundRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::BrightText);
		case Qt::BackgroundColorRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::Dark);
		case RDR_FONT_WEIGHT:
			return QFont::Bold;
		}
		break;

	case RIT_GROUP:
	case RIT_GROUP_BLANK:
	case RIT_GROUP_AGENTS:
	case RIT_GROUP_MY_RESOURCES:
	case RIT_GROUP_NOT_IN_ROSTER:
		switch (ARole)
		{
		case Qt::DisplayRole:
			return AIndex->data(RDR_NAME);
		case Qt::BackgroundColorRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::AlternateBase);
		case RDR_FONT_WEIGHT:
			return QFont::Normal;
		case RDR_GROUP_COUNTER:
			return QString();//groupCounterLabel(AIndex);
		}
		break;

	case RIT_CONTACT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				Jid indexJid = AIndex->data(RDR_FULL_JID).toString();
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
					display = indexJid.bare();
				return display;
			}
		}
		break;

	case RIT_AGENT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
				{
					Jid indexJid = AIndex->data(RDR_FULL_JID).toString();
					display = indexJid.bare();
				}
				return display;
			}
		}
		break;

	case RIT_MY_RESOURCE:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				Jid indexJid = AIndex->data(RDR_FULL_JID).toString();
				return indexJid.resource();
			}
		}
		break;
	}
	return QVariant();
}

bool RostersViewPlugin::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

IRostersView *RostersViewPlugin::rostersView()
{
	return FRostersView;
}

void RostersViewPlugin::startRestoreExpandState()
{
	if (!FStartRestoreExpandState)
	{
		FStartRestoreExpandState = true;
		QTimer::singleShot(0,this,SLOT(onRestoreExpandState()));
	}
}

void RostersViewPlugin::restoreExpandState(const QModelIndex &AParent)
{
	QAbstractItemModel *curModel = FRostersView->model();
	int rows = curModel!=NULL ? curModel->rowCount(AParent) : -1;
	if (rows > 0)
	{
		if (AParent.isValid())
			loadExpandState(AParent);
		for (int row = 0; row<rows; row++)
			restoreExpandState(curModel->index(row,0,AParent));
	}
}

QString RostersViewPlugin::indexGroupName(const QModelIndex &AIndex) const
{
	int indexType = AIndex.data(RDR_TYPE).toInt();
	switch (indexType)
	{
	case RIT_GROUP:
		return AIndex.data(RDR_NAME).toString();
	default:
		return FRostersModel!=NULL ? FRostersModel->singleGroupName(indexType) : QString::null;
	}
	return QString::null;
}

void RostersViewPlugin::loadExpandState(const QModelIndex &AIndex)
{
	QString groupName = indexGroupName(AIndex);
	if (!groupName.isEmpty() || AIndex.data(RDR_TYPE).toInt()==RIT_STREAM_ROOT)
	{
		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		bool isExpanded = FExpandState.value(streamJid).value(groupName,true);
		if (isExpanded && !FRostersView->isExpanded(AIndex))
			FRostersView->expand(AIndex);
		else if (!isExpanded && FRostersView->isExpanded(AIndex))
			FRostersView->collapse(AIndex);
	}
}

void RostersViewPlugin::saveExpandState(const QModelIndex &AIndex)
{
	QString groupName = indexGroupName(AIndex);
	if (!groupName.isEmpty() || AIndex.data(RDR_TYPE).toInt()==RIT_STREAM_ROOT)
	{
		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		if (!FRostersView->isExpanded(AIndex))
			FExpandState[streamJid][groupName] = false;
		else
			FExpandState[streamJid].remove(groupName);
	}
}

QString RostersViewPlugin::groupCounterLabel(const IRosterIndex *AIndex) const
{
	int total =0;
	int active = 0;

	QAbstractItemModel *smodel = FSortFilterProxyModel->sourceModel();
	if (smodel)
	{
		QModelIndex groupIndex = FSortFilterProxyModel->mapToSource(FRostersView->mapToProxy(FSortFilterProxyModel,FRostersView->rostersModel()->modelIndexByRosterIndex(const_cast<IRosterIndex *>(AIndex))));
		for (int row=0; row<smodel->rowCount(groupIndex); row++)
		{
			static const QList<int> countTypes = QList<int>() << RIT_CONTACT << RIT_METACONTACT;
			QModelIndex index = smodel->index(row,0,groupIndex);
			if (countTypes.contains(index.data(RDR_TYPE).toInt()))
			{
				int show = index.data(RDR_SHOW).toInt();
				if (show!=IPresence::Offline && show!=IPresence::Error)
					active++;
				total++;
			}
		}
	}
	return total>0 ? QString("%1/%2").arg(active).arg(total) : QString::null;
}

void RostersViewPlugin::updateGroupCounter(IRosterIndex *AIndex)
{
	IRosterIndex *parent = AIndex->parentIndex();
	while (parent)
	{
		if (GroupsWithCounter.contains(parent->type()))
			emit rosterDataChanged(parent, RDR_GROUP_COUNTER);
		parent = parent->parentIndex();
	}
}

void RostersViewPlugin::onRostersViewDestroyed(QObject *AObject)
{
	Q_UNUSED(AObject);
	FRostersView = NULL;
}

void RostersViewPlugin::onViewModelAboutToBeReset()
{
	if (FRostersView->currentIndex().isValid())
	{
		FViewSavedState.currentIndex = FRostersView->rostersModel()->rosterIndexByModelIndex(FRostersView->mapToModel(FRostersView->currentIndex()));
		FViewSavedState.sliderPos = FRostersView->verticalScrollBar()->sliderPosition();
	}
	else
		FViewSavedState.currentIndex = NULL;
}

void RostersViewPlugin::onViewModelReset()
{
	FRostersView->setRootIndex(FRostersView->model()->index(0,0));
	restoreExpandState();
	if (FViewSavedState.currentIndex != NULL)
	{
		FRostersView->setCurrentIndex(FRostersView->mapFromModel(FRostersView->rostersModel()->modelIndexByRosterIndex(FViewSavedState.currentIndex)));
		FRostersView->verticalScrollBar()->setSliderPosition(FViewSavedState.sliderPos);
	}
}

void RostersViewPlugin::onViewModelAboutToBeChanged(QAbstractItemModel *AModel)
{
	Q_UNUSED(AModel);
	if (FRostersView->model())
	{
		disconnect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),this,SLOT(onViewModelAboutToBeReset()));
		disconnect(FRostersView->model(),SIGNAL(modelReset()),this,SLOT(onViewModelReset()));
		disconnect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),this,SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
	}
}

void RostersViewPlugin::onViewModelChanged(QAbstractItemModel *AModel)
{
	Q_UNUSED(AModel);
	if (FRostersView->model())
	{
		FRostersView->setRootIndex(FRostersView->model()->index(0,0));
		connect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),SLOT(onViewModelAboutToBeReset()));
		connect(FRostersView->model(),SIGNAL(modelReset()),SLOT(onViewModelReset()));
		connect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
		startRestoreExpandState();
	}
}

void RostersViewPlugin::onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
	if (!AParent.isValid())
		FRostersView->setRootIndex(FRostersView->model()->index(0,0));
	if (AStart == 0)
		loadExpandState(AParent);
	for (int row=AStart; row<=AEnd; row++)
		restoreExpandState(AParent.child(row,0));
}

void RostersViewPlugin::onViewIndexCollapsed(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
}

void RostersViewPlugin::onViewIndexExpanded(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
}

void RostersViewPlugin::onRosterModelAboutToBeSet(IRostersModel *AModel)
{
	if (FRostersView->rostersModel())
	{
		disconnect(FRostersView->rostersModel()->instance(),SIGNAL(indexInserted(IRosterIndex *)),this,SLOT(onRosterIndexInserted(IRosterIndex *)));
		disconnect(FRostersView->rostersModel()->instance(),SIGNAL(indexRemoved(IRosterIndex *)),this,SLOT(onRosterIndexRemoved(IRosterIndex *)));
		disconnect(FRostersView->rostersModel()->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),this,SLOT(onRosterIndexDataChanged(IRosterIndex *, int)));
	}
	if (AModel)
	{
		connect(AModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
		connect(AModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRosterIndexRemoved(IRosterIndex *)));
		connect(AModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRosterIndexDataChanged(IRosterIndex *, int)));
	}
}

void RostersViewPlugin::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (GroupsWithCounter.contains(AIndex->type()))
	{
		FRostersView->insertLabel(FGroupCounterLabel,AIndex);
	}
}

void RostersViewPlugin::onRosterIndexRemoved(IRosterIndex *AIndex)
{
	if (GroupsWithCounter.contains(AIndex->type()))
	{
		FRostersView->removeLabel(FGroupCounterLabel,AIndex);
	}
	/*else if (AIndex->type() == RIT_CONTACT)
	{
		updateGroupCounter(AIndex);
	}*/
}

void RostersViewPlugin::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AIndex)
	Q_UNUSED(ARole)
	/*if (AIndex->type()==RIT_CONTACT && ARole==RDR_SHOW)
	{
		updateGroupCounter(AIndex);
	}*/
}

void RostersViewPlugin::onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter)
{
	Jid befour = ARoster->streamJid();
	if (FExpandState.contains(befour))
	{
		QHash<QString, bool> state = FExpandState.take(befour);
		if (befour && AAfter)
			FExpandState.insert(AAfter,state);
	}
}

void RostersViewPlugin::onAccountShown(IAccount *AAccount)
{
	if (AAccount->isActive())
	{
		QByteArray data = Options::fileValue("rosterview.expand-state",AAccount->accountId().toString()).toByteArray();
		QDataStream stream(data);
		stream >> FExpandState[AAccount->xmppStream()->streamJid()];
	}
}

void RostersViewPlugin::onAccountHidden(IAccount *AAccount)
{
	if (AAccount->isActive())
	{
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		stream << FExpandState.take(AAccount->xmppStream()->streamJid());
		Options::setFileValue(data,"rosterview.expand-state",AAccount->accountId().toString());
	}
}

void RostersViewPlugin::onAccountDestroyed(const QUuid &AAccountId)
{
	Options::setFileValue(QVariant(),"rosterview.expand-state",AAccountId.toString());
}

void RostersViewPlugin::onRestoreExpandState()
{
	restoreExpandState();
	FStartRestoreExpandState = false;
}

void RostersViewPlugin::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWSTATUSTEXT));
	onOptionsChanged(Options::node(OPV_ROSTER_SORTBYSTATUS));
	onOptionsChanged(Options::node(OPV_ROSTER_GROUPCONTACTS));
}

void RostersViewPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_SHOWOFFLINE)
	{
		FSortFilterProxyModel->invalidate();
		if (ANode.value().toBool())
			restoreExpandState();
		FShowOfflineAction->setChecked(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_ROSTER_SHOWSTATUSTEXT)
	{
		FRostersView->updateStatusText();
		emit rosterDataChanged(NULL, Qt::DisplayRole);
	}
	else if (ANode.path() == OPV_ROSTER_SORTBYSTATUS)
	{
		FSortFilterProxyModel->invalidate();
	}
	else if (ANode.path() == OPV_ROSTER_GROUPCONTACTS)
	{
		FGroupContactsAction->setChecked(ANode.value().toBool());
	}
}

void RostersViewPlugin::onShowOfflinesAction(bool AChecked)
{
	Options::node(OPV_ROSTER_SHOWOFFLINE).setValue(AChecked);
}

void RostersViewPlugin::onGroupContactsAction(bool AChecked)
{
	Options::node(OPV_ROSTER_GROUPCONTACTS).setValue(AChecked);
}

Q_EXPORT_PLUGIN2(plg_rostersview, RostersViewPlugin)
