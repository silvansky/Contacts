#include "rostersearch.h"

#include <QDesktopServices>
#include <QKeyEvent>
#include <utils/iconstorage.h>

RosterSearch::RosterSearch()
{
	FRostersViewPlugin = NULL;
	FMainWindow = NULL;

	FSearchEdit = NULL;
	FFieldsMenu = NULL;

	FEditTimeout.setSingleShot(true);
	FEditTimeout.setInterval(500);
	connect(&FEditTimeout,SIGNAL(timeout()),SLOT(onEditTimedOut()));

	setDynamicSortFilter(false);
	setFilterCaseSensitivity(Qt::CaseInsensitive);

	searchNotFound = searchInHistory = searchInRambler = 0;
	searchInRamblerLabel = searchInHistoryLabel = searchNotFoundLabel = 0;

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
		{
			connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(labelClicked(IRosterIndex *, int)), SLOT(indexClicked(IRosterIndex *, int)));
			FRostersViewPlugin->rostersView()->instance()->installEventFilter(this);
		}
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
		QFrame * searchFrame = new QFrame(FMainWindow->topToolBarChanger()->toolBar());
		QHBoxLayout * layout = new QHBoxLayout(searchFrame);
		layout->setSpacing(0);
		layout->setMargin(0);
		searchFrame->setLayout(layout);
		searchFrame->setObjectName("searchFrame");
		searchFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		FSearchEdit = new SearchEdit;
		FSearchEdit->setStyleSheet("SearchEdit { border: 0px; }");
		layout->insertWidget(0, FSearchEdit);
		FSearchEdit->setToolTip(tr("Search in roster"));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), &FEditTimeout, SLOT(start()));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), SLOT(onSearchTextChanged(const QString&)));
		FSearchEdit->installEventFilter(this);
		FMainWindow->topToolBarChanger()->insertWidget(searchFrame, TBG_MWTTB_ROSTERSEARCH);
		setSearchEnabled(true);
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
	if (!FSearchEdit->text().isEmpty())
		createSearchLinks();
	else
	{
		destroyNotFoundItem();
		destroySearchLinks();
	}
	invalidate();
	if (FRostersViewPlugin)
		FRostersViewPlugin->restoreExpandState();
	if (!(foundItems || FSearchEdit->text().isEmpty()))
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
		case RIT_GROUP_MY_RESOURCES:
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

bool RosterSearch::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == FSearchEdit)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent * keyEvent = (QKeyEvent*)event;
			if (keyEvent)
			{
				if (keyEvent->key() == Qt::Key_Down)
				{
					IRostersView * view = FRostersViewPlugin->rostersView();
					if (view)
					{
						view->selectFirstItem();
						view->instance()->setFocus();
					}
				}
			}
		}
	}
	if (((obj == FMainWindow->instance()) || (FRostersViewPlugin->rostersView() && (obj == FRostersViewPlugin->rostersView()->instance()))) &&
	    (event->type() == QEvent::KeyPress))
	{
		QKeyEvent * keyEvent = (QKeyEvent*)event;
		if ((keyEvent->key() >= Qt::Key_Space && keyEvent->key() <= Qt::Key_AsciiTilde) || (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::ShiftModifier))
		{
			FSearchEdit->processKeyPressEvent(keyEvent);
			FSearchEdit->setFocus();
		}
	}
	return QSortFilterProxyModel::eventFilter(obj, event);
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
	if (!text.isEmpty())
	{
		destroyNotFoundItem();
		destroySearchLinks();
	}
	else
		createSearchLinks();
}

void RosterSearch::createSearchLinks()
{
	if (rostersModel)
	{
		QString searchText = FSearchEdit->text();
		if (!(searchText.isEmpty() || rostersModel->streams().isEmpty()))
		{
			searchInHistory = rostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInHistory", rostersModel->streamRoot(rostersModel->streams().first()));
			rostersModel->insertRosterIndex(searchInHistory, rostersModel->streamRoot(rostersModel->streams().first()));

			searchInRambler = rostersModel->createRosterIndex(RIT_SEARCH_LINK, "searchInRambler", rostersModel->streamRoot(rostersModel->streams().first()));
			rostersModel->insertRosterIndex(searchInRambler, rostersModel->streamRoot(rostersModel->streams().first()));

			searchInHistory->setData(RDR_SEARCH_LINK, "http://id-planet.rambler.ru");
			searchInHistory->setData(RDR_MOUSE_CURSOR, Qt::PointingHandCursor);
			searchInRambler->setData(RDR_SEARCH_LINK, "http://nova.rambler.ru/search?query=" + searchText);
			searchInRambler->setData(RDR_MOUSE_CURSOR, Qt::PointingHandCursor);

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
	if (rostersModel && !rostersModel->streams().isEmpty())
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
	if (rostersModel && !rostersModel->streams().isEmpty())
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
	if (searchNotFound && rostersModel && !rostersModel->streams().isEmpty())
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
