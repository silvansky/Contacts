#include "rostersearch.h"

#include <QKeyEvent>
#include <QDesktopServices>

RosterSearch::RosterSearch()
{
	FMainWindow = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;

	FSearchEdit = NULL;
	FFieldsMenu = NULL;

	FItemsFound = false;
	FSearchEnabled = false;
	FSearchNotFound = FSearchHistory = FSearchRambler = NULL;

	FEditTimeout.setInterval(500);
	FEditTimeout.setSingleShot(true);
	connect(&FEditTimeout,SIGNAL(timeout()),SLOT(onEditTimedOut()));

	setDynamicSortFilter(false);
	setFilterCaseSensitivity(Qt::CaseInsensitive);
}

RosterSearch::~RosterSearch()
{
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

bool RosterSearch::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin->rostersView())
		{
			FRostersViewPlugin->rostersView()->instance()->installEventFilter(this);
			connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(labelClicked(IRosterIndex *, int)), SLOT(onRosterLabelClicked(IRosterIndex *, int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		IMainWindowPlugin *mainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		if (mainWindowPlugin)
		{
			FMainWindow = mainWindowPlugin->mainWindow();
			FMainWindow->instance()->installEventFilter(this);
		}
	}

	return FRostersViewPlugin!=NULL && FMainWindow!=NULL;
}

bool RosterSearch::initObjects()
{
	if (FMainWindow)
	{
		FFieldsMenu = new Menu(FMainWindow->topToolBarChanger()->toolBar());
		FFieldsMenu->setVisible(false);
		FFieldsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERSEARCH_MENU);

		QFrame *searchFrame = new QFrame(FMainWindow->topToolBarChanger()->toolBar());
		QHBoxLayout *layout = new QHBoxLayout(searchFrame);
		layout->setSpacing(0);
		layout->setMargin(0);
		searchFrame->setLayout(layout);
		searchFrame->setObjectName("searchFrame");
		searchFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		
		FSearchEdit = new SearchEdit;
		layout->insertWidget(0, FSearchEdit);
		FSearchEdit->setToolTip(tr("Search in roster"));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), &FEditTimeout, SLOT(start()));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), SLOT(onSearchTextChanged(const QString&)));
		FSearchEdit->installEventFilter(this);
		
		FMainWindow->topToolBarChanger()->insertWidget(searchFrame, TBG_MWTTB_ROSTERSEARCH);
		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(searchFrame,STS_ROSTERSEARCH_SEARCHFRAME);

		setSearchEnabled(true);
	}

	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
	}

	insertSearchField(RDR_NAME,tr("Name"),true);
	insertSearchField(RDR_JID,tr("Jabber ID"),true);

	return true;
}

int RosterSearch::rosterDataOrder() const
{
	return RDHO_ROSTER_SEARCH;
}

QList<int> RosterSearch::rosterDataRoles() const
{
	static QList<int> dataRoles = QList<int>() 
		<< RDR_FONT_UNDERLINE << RDR_STATES_FORCE_OFF << Qt::ForegroundRole; 
	return dataRoles;
}

QList<int> RosterSearch::rosterDataTypes() const
{
	static QList<int> dataTypes = QList<int>() << RIT_SEARCH_LINK << RIT_SEARCH_EMPTY;
	return dataTypes;
}

QVariant RosterSearch::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	int type = AIndex->data(RDR_TYPE).toInt();
	if (ARole == RDR_FONT_UNDERLINE)
	{
		if (type == RIT_SEARCH_LINK)
		{
			data = true;
		}
	}
	else if (ARole == RDR_STATES_FORCE_OFF)
	{
		static bool block = false;
		if (!block)
		{
			block = true;
			data = AIndex->data(RDR_STATES_FORCE_OFF).toInt() | QStyle::State_Selected | QStyle::State_MouseOver;
			block = false;
		}
	}
	else if (ARole == Qt::ForegroundRole)
	{
		if (type == RIT_SEARCH_LINK)
		{
			if (FRostersViewPlugin)
				data = FRostersViewPlugin->rostersView()->instance()->palette().link().color();
		}
	}
	return data;
}

bool RosterSearch::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

void RosterSearch::startSearch()
{
	FItemsFound = false;
	setFilterRegExp(searchPattern());
	
	if (!searchPattern().isEmpty())
	{
		if (!FItemsFound)
			createNotFoundItem();
		else
			destroyNotFoundItem();
		createSearchLinks();
	}
	else
	{
		destroyNotFoundItem();
		destroySearchLinks();
	}

	emit searchResultUpdated();
}

QString RosterSearch::searchPattern() const
{
	return FSearchEdit->text();
}

void RosterSearch::setSearchPattern(const QString &APattern)
{
	FSearchEdit->setText(APattern);
	emit searchPatternChanged(APattern);
}

bool RosterSearch::isSearchEnabled() const
{
	return FSearchEnabled;
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
		FSearchEnabled = AEnabled;
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
				FItemsFound |= accept;
				return accept;
			}
		case RIT_GROUP:
		case RIT_GROUP_AGENTS:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
		case RIT_GROUP_MY_RESOURCES:
			{
				for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
				{
					if (filterAcceptsRow(childRow,index))
					{
						FItemsFound = true;
						return true;
					}
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

bool RosterSearch::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched == FSearchEdit)
	{
		if (AEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
			if (keyEvent)
			{
				if (keyEvent->key() == Qt::Key_Down)
				{
					FRostersViewPlugin->rostersView()->instance()->setFocus();
				}
			}
		}
	}
	else if (AWatched==(FMainWindow!=NULL ? FMainWindow->instance() : NULL) || AWatched==(FRostersViewPlugin!=NULL ? FRostersViewPlugin->rostersView()->instance() : NULL)) 
	{
		if ( AEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
			if ((keyEvent->key() >= Qt::Key_Space && keyEvent->key() <= Qt::Key_AsciiTilde) || (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::ShiftModifier))
			{
				FSearchEdit->setFocus();
				FSearchEdit->processKeyPressEvent(keyEvent);
			}
		}
	}
	return QSortFilterProxyModel::eventFilter(AWatched, AEvent);
}

void RosterSearch::createSearchLinks()
{
	QString searchText = FSearchEdit->text();
	IRosterIndex *searchRoot = FRostersModel!=NULL ? FRostersModel->streamRoot(FRostersModel->streams().value(0)) : NULL;
	if (searchRoot && !searchText.isEmpty())
	{
		FSearchHistory = FRostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInHistory", searchRoot);
		FSearchHistory->setFlags(Qt::ItemIsEnabled);
		FSearchHistory->setData(Qt::DecorationRole, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTERSEARCH_ICON_GLASS));
		FSearchHistory->setData(Qt::DisplayRole, tr("Search \"%1\" in history").arg(searchText.left(10)));
		FSearchHistory->setData(RDR_SEARCH_LINK, "http://id-planet.rambler.ru");
		FSearchHistory->setData(RDR_MOUSE_CURSOR, Qt::PointingHandCursor);
		FRostersModel->insertRosterIndex(FSearchHistory, searchRoot);

		FSearchRambler = FRostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInRambler", searchRoot);
		FSearchRambler->setFlags(Qt::ItemIsEnabled);
		FSearchRambler->setData(Qt::DecorationRole, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTERSEARCH_ICON_GLASS));
		FSearchRambler->setData(Qt::DisplayRole, tr("Search \"%1\" in Rambler").arg(searchText.left(10)));
		FSearchRambler->setData(RDR_SEARCH_LINK, "http://nova.rambler.ru/search?query=" + searchText);
		FSearchRambler->setData(RDR_MOUSE_CURSOR, Qt::PointingHandCursor);
		FRostersModel->insertRosterIndex(FSearchRambler, searchRoot);
	}
}

void RosterSearch::destroySearchLinks()
{
	if (FSearchHistory)
	{
		FRostersModel->removeRosterIndex(FSearchHistory);
		FSearchHistory = NULL;
	}
	if (FSearchRambler)
	{
		FRostersModel->removeRosterIndex(FSearchRambler);
		FSearchRambler = NULL;
	}
}

void RosterSearch::createNotFoundItem()
{
	IRosterIndex *searchRoot = FRostersModel!=NULL ? FRostersModel->streamRoot(FRostersModel->streams().value(0)) : NULL;
	if (searchRoot)
	{
		FSearchNotFound = FRostersModel->createRosterIndex(RIT_SEARCH_EMPTY, "searchNotFound", searchRoot);
		FSearchNotFound->setFlags(0);
		FSearchNotFound->setData(Qt::DisplayRole, tr("Contacts not found"));
		FRostersModel->insertRosterIndex(FSearchNotFound, searchRoot);
	}
}

void RosterSearch::destroyNotFoundItem()
{
	if (FSearchNotFound)
	{
		FRostersModel->removeRosterIndex(FSearchNotFound);
		FSearchNotFound = NULL;
	}
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
	startSearch();
}

void RosterSearch::onSearchTextChanged(const QString &AText)
{
	Q_UNUSED(AText);
}

void RosterSearch::onRosterLabelClicked(IRosterIndex *AIndex, int ALabelId)
{
	Q_UNUSED(ALabelId);
	if (AIndex == FSearchHistory || AIndex == FSearchRambler)
	{
		QDesktopServices::openUrl(QUrl(AIndex->data(RDR_SEARCH_LINK).toString()));
	}
}

Q_EXPORT_PLUGIN2(plg_rostersearch, RosterSearch)
