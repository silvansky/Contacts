#include "rostersearch.h"

#include <QDesktopServices>
#include <QDebug>

RosterSearch::RosterSearch()
{
	FRostersViewPlugin = NULL;
	FMainWindow = NULL;

	FSearchEdit = NULL;
	FFieldsMenu = NULL;
	//FSearchToolBarChanger = NULL;

	FEditTimeout.setSingleShot(true);
	FEditTimeout.setInterval(500);
	connect(&FEditTimeout,SIGNAL(timeout()),SLOT(onEditTimedOut()));

	setDynamicSortFilter(false);
	setFilterCaseSensitivity(Qt::CaseInsensitive);

	searchNotFound = searchInHistory = searchInRambler = 0;
	searchInRamblerLabel = searchInHistoryLabel = searchNotFoundLabel = 0;

	/*
	QToolBar *searchToolBar = new QToolBar(tr("Search toolbar"));
	searchToolBar->setAllowedAreas(Qt::TopToolBarArea);
	searchToolBar->setMovable(false);
	FSearchToolBarChanger = new ToolBarChanger(searchToolBar);
	FSearchToolBarChanger->setManageVisibility(false);
	FSearchToolBarChanger->setSeparatorsVisible(false);
	*/
	//FSearchToolBarChanger->insertAction(FFieldsMenu->menuAction());

	foundItems = 0;
}

RosterSearch::~RosterSearch()
{
	if (searchNotFoundLabel)
		FRostersViewPlugin->rostersView()->removeIndexLabel(searchNotFoundLabel, searchNotFound);
	if (searchInHistoryLabel)
		FRostersViewPlugin->rostersView()->removeIndexLabel(searchInHistoryLabel, searchInHistory);
	if (searchInRamblerLabel)
		FRostersViewPlugin->rostersView()->removeIndexLabel(searchInRamblerLabel, searchInRambler);
	destroySearchLinks();
	destroyNotFoundItem();
}

void RosterSearch::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Search");
	APluginInfo->description = tr("Allows to search for contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(ROSTERSVIEW_UUID);
	APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool RosterSearch::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin->rostersView())
			connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(labelClicked(IRosterIndex *, int)), SLOT(indexClicked(IRosterIndex *, int)));
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		rostersModel = qobject_cast<IRostersModel*>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		IMainWindowPlugin *mainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		if (mainWindowPlugin)
		{
			FMainWindow = mainWindowPlugin->mainWindow();
		}
	}

	return FRostersViewPlugin!=NULL && FMainWindow!=NULL;
}

bool RosterSearch::initObjects()
{
	if (FMainWindow)
	{
		/*
		Action *searchAction = new Action(FMainWindow->topToolBarChanger());
		searchAction->setIcon(RSR_STORAGE_MENUICONS,MNI_ROSTERSEARCH_MENU);
		searchAction->setToolTip(tr("Show search toolbar"));
		searchAction->setCheckable(true);
		connect(searchAction,SIGNAL(triggered(bool)),SLOT(onSearchActionTriggered(bool)));
		*/
		//FMainWindow->topToolBarChanger()->insertAction(searchAction, TBG_MWTTB_ROSTERSEARCH);

		//FMainWindow->instance()->addToolBar(FSearchToolBarChanger->toolBar());
		//FMainWindow->instance()->insertToolBarBreak(FSearchToolBarChanger->toolBar());

		//FSearchToolBarChanger->toolBar()->setVisible(false);
		FFieldsMenu = new Menu(FMainWindow->topToolBarChanger()->toolBar());
		FFieldsMenu->setVisible(false);
		FFieldsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERSEARCH_MENU);
		FSearchEdit = new QLineEdit(FMainWindow->topToolBarChanger()->toolBar());
		FSearchEdit->setToolTip(tr("Search in roster"));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), &FEditTimeout, SLOT(start()));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), SLOT(onSearchTextChanged(const QString&)));
		FMainWindow->topToolBarChanger()->insertWidget(FSearchEdit);
		setSearchEnabled(false);
	}
	if (FRostersViewPlugin)
		FRostersViewPlugin->rostersView()->insertProxyModel(this, RPO_ROSTERSEARCH_FILTER);

	insertSearchField(RDR_NAME,tr("Name"),true);
	insertSearchField(RDR_STATUS,tr("Status"),true);
	insertSearchField(RDR_JID,tr("Jabber ID"),true);

	return true;
}

void RosterSearch::startSearch()
{
	foundItems = 0;
	setFilterRegExp(FSearchEdit->text());
	invalidate();
	createSearchLinks();
	if (FRostersViewPlugin)
		FRostersViewPlugin->restoreExpandState();
	if (!foundItems)
		createNotFoundItem();
	emit searchResultUpdated();
}

QString RosterSearch::searchPattern() const
{
	return filterRegExp().pattern();
}

void RosterSearch::setSearchPattern(const QString &APattern)
{
	FSearchEdit->setText(APattern);
	emit searchPatternChanged(APattern);
}

bool RosterSearch::isSearchEnabled() const
{
	return searchEnabled;
}

void RosterSearch::setSearchEnabled(bool AEnabled)
{
	if (isSearchEnabled() != AEnabled)
	{
		if (FRostersViewPlugin)
		{
			if (AEnabled)
				FRostersViewPlugin->rostersView()->insertProxyModel(this,RPO_ROSTERSEARCH_FILTER);
			else
				FRostersViewPlugin->rostersView()->removeProxyModel(this);
		}
		searchEnabled = AEnabled;
		emit searchStateChanged(AEnabled);
	}
}

void RosterSearch::insertSearchField(int ADataRole, const QString &AName, bool AEnabled)
{
	Action *action = FFieldActions.value(ADataRole,NULL);
	if (action == NULL)
	{
		action = new Action(FFieldsMenu);
		connect(action,SIGNAL(triggered(bool)),SLOT(onFieldActionTriggered(bool)));
		FFieldActions.insert(ADataRole,action);
		FFieldsMenu->addAction(action,AG_DEFAULT,true);
	}
	action->setText(AName);
	action->setCheckable(true);
	action->setChecked(AEnabled);
	emit searchFieldInserted(ADataRole,AName);
}

Menu *RosterSearch::searchFieldsMenu() const
{
	return FFieldsMenu;
}

QList<int> RosterSearch::searchFields() const
{
	return FFieldActions.keys();
}

bool RosterSearch::isSearchFieldEnabled(int ADataRole) const
{
	return FFieldActions.contains(ADataRole) && FFieldActions.value(ADataRole)->isChecked();
}

void RosterSearch::setSearchFieldEnabled(int ADataRole, bool AEnabled)
{
	if (FFieldActions.contains(ADataRole))
	{
		FFieldActions.value(ADataRole)->setChecked(AEnabled);
		emit searchFieldChanged(ADataRole);
	}
}

void RosterSearch::removeSearchField(int ADataRole)
{
	if (FFieldActions.contains(ADataRole))
	{
		Action *action = FFieldActions.take(ADataRole);
		FFieldsMenu->removeAction(action);
		delete action;
		emit searchFieldRemoved(ADataRole);
	}
}

bool RosterSearch::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
	if (!searchPattern().isEmpty())
	{
		QModelIndex index = sourceModel()!=NULL ? sourceModel()->index(ARow,0,AParent) : QModelIndex();
		switch (index.data(RDR_TYPE).toInt())
		{
		case RIT_CONTACT:
		case RIT_AGENT:
		case RIT_MY_RESOURCE:
			{
				bool accept = true;
				foreach(int dataField, FFieldActions.keys())
				{
					if (isSearchFieldEnabled(dataField))
					{
						accept = false;
						if (filterRegExp().indexIn(index.data(dataField).toString())>=0)
							return true;
					}
				}
				if (accept)
					foundItems++;
				return accept;
			}
		case RIT_GROUP:
		case RIT_GROUP_AGENTS:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
			{
				for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
					if (filterAcceptsRow(childRow,index))
					{
					foundItems++;
					return true;
				}
				return false;
			}
		case RIT_SEARCH_LINK:
		case RIT_SEARCH_EMPTY:
			return true;
		}
	}
	return true;
}

void RosterSearch::onFieldActionTriggered(bool)
{
	startSearch();
}

void RosterSearch::onSearchActionTriggered(bool AChecked)
{
	setSearchEnabled(AChecked);
}

void RosterSearch::onEditTimedOut()
{
	emit searchPatternChanged(FSearchEdit->text());
	startSearch();
}

void RosterSearch::onSearchTextChanged(const QString &text)
{
	setSearchEnabled(!text.isEmpty());
	if (!searchEnabled)
	{
		destroyNotFoundItem();
		destroySearchLinks();
	}
}

void RosterSearch::createSearchLinks()
{
	if (rostersModel)
	{
		//destroySearchLinks();
		QString searchText = FSearchEdit->text();
		qDebug() << "searchText == " + searchText;

		if (!searchText.isEmpty())
		{

			searchInHistory = rostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInHistory", rostersModel->streamRoot(rostersModel->streams().first()));
			rostersModel->insertRosterIndex(searchInHistory, rostersModel->streamRoot(rostersModel->streams().first()));

			searchInRambler = rostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInRambler", rostersModel->streamRoot(rostersModel->streams().first()));
			rostersModel->insertRosterIndex(searchInRambler, rostersModel->streamRoot(rostersModel->streams().first()));

			searchInHistory->setData(RDR_SEARCH_LINK, "http://rambler.ru");
			searchInRambler->setData(RDR_SEARCH_LINK, "http://nova.rambler.ru/search?query=" + searchText);

			if (!searchInHistoryLabel)
				searchInHistoryLabel = FRostersViewPlugin->rostersView()->createIndexLabel(0, "");
			FRostersViewPlugin->rostersView()->updateIndexLabel(searchInHistoryLabel, tr("Search \"%1\" in history").arg(searchText), IRostersView::LabelVisible);
			FRostersViewPlugin->rostersView()->insertIndexLabel(searchInHistoryLabel, searchInHistory);
			if (!searchInRamblerLabel)
				searchInRamblerLabel = FRostersViewPlugin->rostersView()->createIndexLabel(0, tr("Search") + " \"" + searchText + "\" " + tr("in Rambler"), IRostersView::LabelVisible);
			FRostersViewPlugin->rostersView()->updateIndexLabel(searchInRamblerLabel, tr("Search \"%1\" in Rambler").arg(searchText), IRostersView::LabelVisible);
			FRostersViewPlugin->rostersView()->insertIndexLabel(searchInRamblerLabel, searchInRambler);
		}
	}
}

void RosterSearch::destroySearchLinks()
{
	if (rostersModel)
	{
		if (searchInHistory)
		{
			rostersModel->removeRosterIndex(rostersModel->findRosterIndex(searchInHistory->type(), searchInHistory->id(), rostersModel->streamRoot(rostersModel->streams().first())));
			searchInHistory = NULL;
		}
		if (searchInRambler)
		{
			rostersModel->removeRosterIndex(rostersModel->findRosterIndex(searchInRambler->type(), searchInRambler->id(), rostersModel->streamRoot(rostersModel->streams().first())));
			searchInRambler = NULL;
		}
	}
}

void RosterSearch::createNotFoundItem()
{
	if (rostersModel)
	{
		destroyNotFoundItem();
		searchNotFound = rostersModel->createRosterIndex(RIT_SEARCH_EMPTY, "searchNotFound", rostersModel->streamRoot(rostersModel->streams().first()));
		rostersModel->insertRosterIndex(searchNotFound, rostersModel->streamRoot(rostersModel->streams().first()));
		if (!searchNotFoundLabel)
			searchNotFoundLabel = FRostersViewPlugin->rostersView()->createIndexLabel(0, tr("Contacts not found"), IRostersView::LabelVisible);
		FRostersViewPlugin->rostersView()->insertIndexLabel(searchNotFoundLabel, searchNotFound);
	}
}

void RosterSearch::destroyNotFoundItem()
{
	if (searchNotFound && rostersModel)
	{
		rostersModel->removeRosterIndex(rostersModel->findRosterIndex(searchNotFound->type(), searchNotFound->id(), rostersModel->streamRoot(rostersModel->streams().first())));
		searchNotFound = NULL;
	}
}

void RosterSearch::indexClicked(IRosterIndex *AIndex, int ALabelId)
{
	Q_UNUSED(ALabelId);
	if (AIndex == searchInHistory || AIndex == searchInRambler)
	{
		QDesktopServices::openUrl(QUrl(AIndex->data(RDR_SEARCH_LINK).toString()));
	}
}

Q_EXPORT_PLUGIN2(plg_rostersearch, RosterSearch)
