#ifndef NOGROUPSPROXYMODEL_H
#define NOGROUPSPROXYMODEL_H

#include <QSize>
#include <QStringList>
#include <QAbstractProxyModel>

struct Mapping 
{
	
};

class NoGroupsProxyModel : 
	public QAbstractProxyModel
{
	Q_OBJECT;
public:
	NoGroupsProxyModel(QObject *AParent = NULL);
	~NoGroupsProxyModel();
	//QAbstractItemModel
	virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &AIndex) const;
	virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
	virtual bool hasChildren(const QModelIndex &AParent);
	virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &AIndex, const QVariant &AValue, int ARole = Qt::EditRole);
	virtual QVariant headerData(int ASection, Qt::Orientation AOrientation, int ARole = Qt::DisplayRole) const;
	virtual bool setHeaderData(int ASection, Qt::Orientation AOrientation, const QVariant &AValue, int ARole = Qt::EditRole);
	virtual QStringList mimeTypes() const;
	virtual QMimeData *mimeData(const QModelIndexList &AIndexes) const;
	virtual bool dropMimeData(const QMimeData *AData, Qt::DropAction AAction,int ARow, int AColumn, const QModelIndex &AParent);
	virtual Qt::DropActions supportedDropActions() const;
	virtual bool insertRows(int ARow, int ACount, const QModelIndex &AParent = QModelIndex());
	virtual bool insertColumns(int AColumn, int ACount, const QModelIndex &AParent = QModelIndex());
	virtual bool removeRows(int ARow, int ACount, const QModelIndex &AParent = QModelIndex());
	virtual bool removeColumns(int AColumn, int ACount, const QModelIndex &AParent = QModelIndex());
	virtual void fetchMore(const QModelIndex &AParent);
	virtual bool canFetchMore(const QModelIndex &AParent) const;
	virtual Qt::ItemFlags flags(const QModelIndex &AIndex) const;
	virtual QModelIndex buddy(const QModelIndex &AIndex) const;
	virtual QModelIndexList match(const QModelIndex &AStart, int ARole,const QVariant &AValue, int AHits = 1,
		Qt::MatchFlags AFlags = Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
	virtual QSize span(const QModelIndex &AIndex) const;
	virtual void sort(int AColumn, Qt::SortOrder AOrder = Qt::AscendingOrder);
	//QAbstractProxyModel
	virtual QModelIndex mapFromSource(const QModelIndex &ASourceIndex) const;
	virtual QModelIndex mapToSource(const QModelIndex &AProxyIndex) const;
	virtual void setSourceModel(QAbstractItemModel *ASourceModel);
   // NoGroupsProxyModel
	bool isGroupIndex(const QModelIndex &AIndex) const;
protected slots:
	void onSourceAboutToBeReset();
	void onSourceReset();
	void onSourceLayoutAboutToBeChanged();
	void onSourceLayoutChanged();
	void onSourceDataChanged(const QModelIndex &ASourceTopLeft, const QModelIndex &ASourceBottomRight);
	void onSourceHeaderDataChanged(Qt::Orientation AOrientation, int AStart, int AEnd);
	void onSourceRowsAboutToBeInserted(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceRowsInserted(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceRowsAboutToBeRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceRowsRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceColumnsAboutToBeInserted(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceColumnsInserted(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceColumnsAboutToBeRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd);
	void onSourceColumnsRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd);
private:
	QAbstractItemModel *FModel;
};

#endif // NOGROUPSPROXYMODEL_H
