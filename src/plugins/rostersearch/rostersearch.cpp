#include "rostersearch.h"

#include <QKeyEvent>
#include <QDesktopServices>
#include <QItemSelectionModel>
#include <definitions/rosterclickhookerorders.h>

RosterSearch::RosterSearch()
{
	FMainWindow = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;

	FSearchEdit = NULL;
	FSearchFieldsMenu = new Menu;

	FItemsFound = false;
	FSearchStarted = false;
	FSearchEnabled = false;

	FSearchNotFound = NULL;
	FSearchHistory = NULL;
	FSearchRambler = NULL;

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
	delete FSearchFieldsMenu;
}

void RosterSearch::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Search");
	APluginInfo->description = tr("Allows to search for contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://friends.rambler.ru";
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
			connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(activated(const QModelIndex &)), SLOT(onRosterIndexActivated(const QModelIndex &)));
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

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FRostersViewPlugin && FMainWindow;
}

bool RosterSearch::initObjects()
{
	if (FMainWindow)
	{
		FSearchFieldsMenu->setParent(FMainWindow->topToolBarChanger()->toolBar());
		FSearchFieldsMenu->setVisible(false);
		FSearchFieldsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERSEARCH_MENU);

		QFrame *searchFrame = new QFrame(FMainWindow->topToolBarChanger()->toolBar());
		FMainWindow->topToolBarChanger()->toolBar()->setObjectName("searchBar");
		QHBoxLayout *layout = new QHBoxLayout(searchFrame);
		layout->setSpacing(0);
		layout->setMargin(0);
		searchFrame->setLayout(layout);
		searchFrame->setObjectName("searchFrame");
		searchFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		FSearchEdit = new SearchEdit;
		FSearchEdit->setObjectName("searchEdit");
		layout->insertWidget(0, FSearchEdit);
		FSearchEdit->setToolTip(tr("Search in contact list"));
		FSearchEdit->setPlaceholderText(tr("Search"));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), &FEditTimeout, SLOT(start()));
		connect(FSearchEdit, SIGNAL(textChanged(const QString &)), SLOT(onSearchTextChanged(const QString&)));
		FSearchEdit->installEventFilter(this);

		FMainWindow->topToolBarChanger()->insertWidget(searchFrame, TBG_MWTTB_ROSTERSEARCH);
		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(FMainWindow->topToolBarChanger()->toolBar(),STS_ROSTERSEARCH_SEARCHFRAME);

		setSearchEnabled(true);
	}

	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
	}

	if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
	{
		FRostersViewPlugin->rostersView()->insertClickHooker(RCHO_DEFAULT, this);
	}

	setSearchField(RDR_NAME,tr("Name"),true);
	setSearchField(RDR_PREP_BARE_JID,tr("Address"),true);

	return true;
}

int RosterSearch::rosterDataOrder() const
{
	return RDHO_ROSTER_SEARCH;
}

QList<int> RosterSearch::rosterDataRoles() const
{
	static QList<int> dataRoles = QList<int>()
		<< RDR_FONT_UNDERLINE << RDR_STATES_FORCE_OFF << RDR_FOOTER_TEXT << Qt::ForegroundRole;
	return dataRoles;
}

QList<int> RosterSearch::rosterDataTypes() const
{
	static QList<int> dataTypes = QList<int>() << RIT_CONTACT << RIT_METACONTACT << RIT_SEARCH_LINK << RIT_SEARCH_EMPTY;
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
		if (type==RIT_SEARCH_LINK || type==RIT_SEARCH_EMPTY)
		{
			static bool block = false;
			if (!block)
			{
				block = true;
				data = AIndex->data(RDR_STATES_FORCE_OFF).toInt() | /*QStyle::State_Selected |*/ QStyle::State_MouseOver;
				block = false;
			}
		}
	}
	else if (ARole == RDR_FOOTER_TEXT)
	{
		if (type==RIT_CONTACT || type==RIT_METACONTACT)
		{
			static bool block = false;
			if (!block && FRostersModel && FSearchStarted)
			{
				block = true;
				int field = findAcceptableField(FRostersModel->modelIndexByRosterIndex(const_cast<IRosterIndex *>(AIndex)));
				if ((field >= 0) && (field != RDR_NAME))
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					QString fieldValue = findFieldMatchedValue(AIndex,field);
					QString note = QString("%1: %2").arg(FSearchFields.value(field).name).arg(fieldValue);
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),note);
					data = footer;
				}
				block = false;
			}
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
		if (!FSearchStarted && FRostersViewPlugin)
		{
			FLastShowOffline = Options::node(OPV_ROSTER_SHOWOFFLINE).value().toBool();
			Options::node(OPV_ROSTER_SHOWOFFLINE).setValue(true);
			FRostersViewPlugin->rostersView()->instance()->expandAll();
			FRostersViewPlugin->rostersView()->instance()->setItemsExpandable(false);
			FRostersViewPlugin->rostersView()->selectRow(1);
		}
		FSearchStarted = true;
	}
	else
	{
		destroyNotFoundItem();
		destroySearchLinks();
		if (FSearchStarted && FRostersViewPlugin)
		{
			FRostersViewPlugin->startRestoreExpandState();
			FRostersViewPlugin->rostersView()->instance()->setItemsExpandable(true);
			Options::node(OPV_ROSTER_SHOWOFFLINE).setValue(FLastShowOffline);
		}
		FSearchStarted = false;
	}

	emit searchResultUpdated();
	emit rosterDataChanged(NULL, RDR_FOOTER_TEXT);
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

Menu *RosterSearch::searchFieldsMenu() const
{
	return FSearchFieldsMenu;
}

QList<int> RosterSearch::searchFields() const
{
	return FSearchFields.keys();
}

bool RosterSearch::isSearchFieldEnabled(int ADataRole) const
{
	return FSearchFields.value(ADataRole).enabled;
}

QString RosterSearch::searchFieldName(int ADataRole) const
{
	return FSearchFields.value(ADataRole).name;
}

void RosterSearch::setSearchField(int ADataRole, const QString &AName, bool AEnabled)
{
	if (ADataRole>=0 && !AName.isEmpty())
	{
		SearchField &field = FSearchFields[ADataRole];
		field.name = AName;
		field.enabled = AEnabled;
		if (field.action == NULL)
		{
			field.action = new Action(FSearchFieldsMenu);
			field.action->setCheckable(true);
			connect(field.action,SIGNAL(triggered(bool)),SLOT(onFieldActionTriggered(bool)));
			FSearchFieldsMenu->addAction(field.action,AG_DEFAULT,true);
		}
		field.action->setText(AName);
		field.action->setChecked(AEnabled);
		emit searchFieldChanged(ADataRole);
	}
}

void RosterSearch::removeSearchField(int ADataRole)
{
	if (FSearchFields.contains(ADataRole))
	{
		SearchField field = FSearchFields.take(ADataRole);
		FSearchFieldsMenu->removeAction(field.action);
		delete field.action;
		emit searchFieldRemoved(ADataRole);
	}
}

bool RosterSearch::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	Q_UNUSED(AIndex)
	Q_UNUSED(AOrder)
	if (!FSearchEdit->text().isEmpty())
		FSearchEdit->setText(QString::null);
	return false;
}

bool RosterSearch::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
	if (!searchPattern().isEmpty())
	{
		QModelIndex index = sourceModel() ? sourceModel()->index(ARow,0,AParent) : QModelIndex();
		switch (index.data(RDR_TYPE).toInt())
		{
		case RIT_CONTACT:
		case RIT_METACONTACT:
			{
				bool accept = findAcceptableField(index)>=0;
				FItemsFound |= accept;
				return accept;
			}
		case RIT_GROUP:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
			{
				for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
					if (filterAcceptsRow(childRow,index))
						return true;
				return false;
			}
		case RIT_ROOT:
		case RIT_STREAM_ROOT:
		case RIT_SEARCH_LINK:
		case RIT_SEARCH_EMPTY:
			return true;
		default:
			return false;
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
	else if (AWatched==(FMainWindow ? FMainWindow->instance() : NULL) || AWatched==(FRostersViewPlugin ? FRostersViewPlugin->rostersView()->instance() : NULL))
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

int RosterSearch::findAcceptableField(const QModelIndex &AIndex) const
{
	const QString pattern = searchPattern();
	for(QMap<int,SearchField>::const_iterator it = FSearchFields.constBegin(); it!=FSearchFields.constEnd(); it++)
	{
		if (it->enabled)
		{
			QVariant field = AIndex.data(it.key());
			if (field.type()==QVariant::StringList && field.toStringList().join(" ").contains(pattern))
				return it.key();
			else if (field.toString().contains(pattern,Qt::CaseInsensitive))
				return it.key();
		}
	}
	return -1;
}

QString RosterSearch::findFieldMatchedValue(const IRosterIndex *AIndex, int AField) const
{
	QString fieldValue;
	if (FSearchFields.contains(AField))
	{
		QVariant fieldData = AIndex->data(AField);
		fieldValue = fieldData.toString();
		if (fieldValue.isEmpty())
		{
			if (fieldData.type() == QVariant::StringList)
			{
				foreach(QString listItem, fieldData.toStringList())
				{
					if (listItem.contains(searchPattern(),Qt::CaseInsensitive))
						return listItem;
				}
			}
		}
	}
	return fieldValue;
}

void RosterSearch::createSearchLinks()
{
	QString searchText = FSearchEdit->text();
	IRosterIndex *searchRoot = FRostersModel!=NULL ? FRostersModel->streamRoot(FRostersModel->streams().value(0)) : NULL;
	if (searchRoot && !searchText.isEmpty())
	{
		if (!FSearchHistory)
			FSearchHistory = FRostersModel->createRosterIndex(RIT_SEARCH_LINK, searchRoot);
		FSearchHistory->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		FSearchHistory->setData(Qt::DecorationRole, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTERSEARCH_ICON_GLASS));
		FSearchHistory->setData(Qt::DisplayRole, tr("Search \"%1\" in history").arg(searchText.left(10)));
		FSearchHistory->setData(RDR_TYPE_ORDER,RITO_SEARCH);
		FSearchHistory->setData(RDR_SEARCH_LINK, "http://id-planet.rambler.ru");
		FSearchHistory->setData(RDR_MOUSE_CURSOR, Qt::PointingHandCursor);
		FRostersModel->insertRosterIndex(FSearchHistory, searchRoot);

		if (!FSearchRambler)
			FSearchRambler = FRostersModel->createRosterIndex(RIT_SEARCH_LINK, searchRoot);
		FSearchRambler->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		FSearchRambler->setData(Qt::DecorationRole, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTERSEARCH_ICON_GLASS));
		FSearchRambler->setData(Qt::DisplayRole, tr("Search \"%1\" in Rambler").arg(searchText.left(10)));
		FSearchRambler->setData(RDR_TYPE_ORDER,RITO_SEARCH);
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
		if (!FSearchNotFound)
			FSearchNotFound = FRostersModel->createRosterIndex(RIT_SEARCH_EMPTY, searchRoot);
		FSearchNotFound->setFlags(0);
		FSearchNotFound->setData(Qt::DisplayRole, tr("Contacts not found"));
		FSearchNotFound->setData(RDR_TYPE_ORDER,RITO_SEARCH_NOT_FOUND);
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

void RosterSearch::onRosterIndexActivated(const QModelIndex &AIndex)
{
	IRosterIndex *index = FRostersModel!=NULL ? FRostersModel->rosterIndexByModelIndex(FRostersViewPlugin->rostersView()->mapToModel(AIndex)) : NULL;
	onRosterLabelClicked(index,0);
}

void RosterSearch::onRosterLabelClicked(IRosterIndex *AIndex, int ALabelId)
{
	Q_UNUSED(ALabelId);
	if (AIndex == FSearchHistory || AIndex == FSearchRambler)
	{
		QDesktopServices::openUrl(QUrl(AIndex->data(RDR_SEARCH_LINK).toString()));
	}
}

void RosterSearch::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_SHOWOFFLINE)
	{
		if (!searchPattern().isEmpty() && !ANode.value().toBool())
			Options::node(OPV_ROSTER_SHOWOFFLINE).setValue(true);
	}
}

Q_EXPORT_PLUGIN2(plg_rostersearch, RosterSearch)
