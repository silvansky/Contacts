#ifndef ROSTERSEARCH_H
#define ROSTERSEARCH_H

#include <QTimer>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/optionvalues.h>
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

struct SearchField
{
	SearchField() {
		enabled = false;
		action = NULL;
	}
	bool enabled;
	QString name;
	Action *action;
};

class RosterSearch :
	public QSortFilterProxyModel,
	public IPlugin,
	public IRosterSearch,
	public IRosterDataHolder,
	public IRostersClickHooker
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IRosterSearch IRosterDataHolder IRostersClickHooker)
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
	//IRostersClickHooker
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
	//IRosterSearch
	virtual void startSearch();
	virtual QString searchPattern() const;
	virtual void setSearchPattern(const QString &APattern);
	virtual bool isSearchEnabled() const;
	virtual void setSearchEnabled(bool AEnabled);
	virtual Menu *searchFieldsMenu() const;
	virtual QList<int> searchFields() const;
	virtual bool isSearchFieldEnabled(int ADataRole) const;
	virtual QString searchFieldName(int ADataRole) const;
	virtual void setSearchField(int ADataRole, const QString &AName, bool AEnabled);
	virtual void removeSearchField(int ADataRole);
signals:
	void searchResultUpdated();
	void searchStateChanged(bool AEnabled);
	void searchPatternChanged(const QString &APattern);
	void searchFieldChanged(int ADataRole);
	void searchFieldRemoved(int ADataRole);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	QRegExp searchRegExp(const QString &APattern) const;
	int findAcceptableField(const QModelIndex &AIndex) const;
	QString findFieldMatchedValue(const IRosterIndex *AIndex, int AField) const;
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
	void onRosterIndexActivated(const QModelIndex &AIndex);
	void onRosterLabelClicked(IRosterIndex *AIndex, int ALabelId);
	void onRosterIndexDestroyed(IRosterIndex *AIndex);
	void onRosterStreamRemoved(const Jid &AStreamJid);
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IMainWindow *FMainWindow;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	IRosterIndex *FSearchHistory;
	IRosterIndex *FSearchRambler;
	IRosterIndex *FSearchNotFound;
private:
	bool FSearchEnabled;
	bool FSearchStarted;
	bool FLastShowOffline;
	mutable bool FItemsFound;
	QTimer FEditTimeout;
	SearchEdit *FSearchEdit;
	Menu *FSearchFieldsMenu;
	QMap<int, SearchField> FSearchFields;
};

#endif // ROSTERSEARCH_H
