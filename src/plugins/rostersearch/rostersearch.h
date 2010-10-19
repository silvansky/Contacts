#ifndef ROSTERSEARCH_H
#define ROSTERSEARCH_H

#include <QTimer>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/rosterdataholderorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterproxyorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersearch.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/action.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/toolbarchanger.h>
#include "searchedit.h"

class RosterSearch :
			public QSortFilterProxyModel,
			public IPlugin,
			public IRosterSearch,
			public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterSearch IRosterDataHolder);
public:
	RosterSearch();
	~RosterSearch();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return ROSTERSEARCH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRosterSearch
	virtual void startSearch();
	virtual QString searchPattern() const;
	virtual void setSearchPattern(const QString &APattern);
	virtual bool isSearchEnabled() const;
	virtual void setSearchEnabled(bool AEnabled);
	virtual void insertSearchField(int ADataRole, const QString &AName, bool AEnabled);
	virtual Menu *searchFieldsMenu() const;
	virtual QList<int> searchFields() const;
	virtual bool isSearchFieldEnabled(int ADataRole) const;
	virtual void setSearchFieldEnabled(int ADataRole, bool AEnabled);
	virtual void removeSearchField(int ADataRole);
signals:
	void searchResultUpdated();
	void searchStateChanged(bool AEnabled);
	void searchPatternChanged(const QString &APattern);
	void searchFieldInserted(int ADataRole, const QString &AName);
	void searchFieldChanged(int ADataRole);
	void searchFieldRemoved(int ADataRole);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void createSearchLinks();
	void destroySearchLinks();
	void createNotFoundItem();
	void destroyNotFoundItem();
protected slots:
	void onFieldActionTriggered(bool);
	void onSearchActionTriggered(bool AChecked);
	void onEditTimedOut();
	void onSearchTextChanged(const QString &text);
	void onRosterLabelClicked(IRosterIndex *AIndex, int ALabelId);
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IMainWindow *FMainWindow;
	IRostersModel * FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	IRosterIndex *FSearchHistory;
	IRosterIndex *FSearchRambler;
	IRosterIndex *FSearchNotFound;
private:
	bool FSearchEnabled;
	mutable bool FItemsFound;
	bool FLastShowOffline;
	Menu *FFieldsMenu;
	QTimer FEditTimeout;
	SearchEdit *FSearchEdit;
	QMap<int, Action *> FFieldActions;
};

#endif // ROSTERSEARCH_H
