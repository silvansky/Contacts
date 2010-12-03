#ifndef METAPROXYMODEL_H
#define METAPROXYMODEL_H

#include <QTimer>
#include <QSortFilterProxyModel>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdataholderorders.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/ipresence.h>

class MetaProxyModel : 
	public QSortFilterProxyModel,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IRosterDataHolder);
public:
	MetaProxyModel(IMetaContacts *AMetaContacts, IRostersView *ARostersView);
	~MetaProxyModel();
	virtual QObject *instance() { return this; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	virtual bool filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const;
protected slots:
	void onInvalidateTimerTimeout();
	void onRostersModelSet(IRostersModel *AModel);
	void onMetaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
private:
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IMetaContacts *FMetaContacts;
private:
	QTimer FInvalidateTimer;
};

#endif // METAPROXYMODEL_H