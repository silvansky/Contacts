#ifndef IROSTERSEARCH_H
#define IROSTERSEARCH_H

#include <utils/menu.h>

#define ROSTERSEARCH_UUID   "{67a6e855-83d1-4a93-a280-fca176108e92}"

class IRosterSearch {
public:
	virtual QObject *instance() =0;
	virtual void startSearch() =0;
	virtual QString searchPattern() const =0;
	virtual void setSearchPattern(const QString &APattern) =0;
	virtual bool isSearchEnabled() const =0;
	virtual void setSearchEnabled(bool AEnabled) =0;
	virtual void insertSearchField(int ADataRole, const QString &AName, bool AEnabled) =0;
	virtual Menu *searchFieldsMenu() const =0;
	virtual QList<int> searchFields() const =0;
	virtual bool isSearchFieldEnabled(int ADataRole) const =0;
	virtual void setSearchFieldEnabled(int ADataRole, bool AEnabled) =0;
	virtual void removeSearchField(int ADataRole) =0;
protected:
	virtual void searchResultUpdated() =0;
	virtual void searchStateChanged(bool AEnabled) =0;
	virtual void searchPatternChanged(const QString &APattern) =0;
	virtual void searchFieldInserted(int ADataRole, const QString &AName) =0;
	virtual void searchFieldChanged(int ADataRole) =0;
	virtual void searchFieldRemoved(int ADataRole) =0;
};

Q_DECLARE_INTERFACE(IRosterSearch,"Virtus.Plugin.IRosterSearch/1.0")

#endif
