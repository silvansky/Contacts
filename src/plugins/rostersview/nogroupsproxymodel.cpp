#include "nogroupsproxymodel.h"

#include <definitions/rosterindextyperole.h>

class EmptyItemModel : public QAbstractItemModel
{
public:
	explicit EmptyItemModel(QObject *AParent = 0) : QAbstractItemModel(AParent) {}
	QModelIndex index(int, int, const QModelIndex &) const { return QModelIndex(); }
	QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
	int rowCount(const QModelIndex &) const { return 0; }
	int columnCount(const QModelIndex &) const { return 0; }
	bool hasChildren(const QModelIndex &) const { return false; }
	QVariant data(const QModelIndex &, int) const { return QVariant(); }
};
Q_GLOBAL_STATIC(EmptyItemModel, staticEmptyModel);

NoGroupsProxyModel::NoGroupsProxyModel(QObject *AParent) : QAbstractProxyModel(AParent)
{
	FModel = NULL;
	setSourceModel(staticEmptyModel());
}

NoGroupsProxyModel::~NoGroupsProxyModel()
{

}

QModelIndex NoGroupsProxyModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
	return QModelIndex();
}

QModelIndex NoGroupsProxyModel::parent(const QModelIndex &AIndex) const
{
	return QModelIndex();
}

int NoGroupsProxyModel::rowCount(const QModelIndex &AParent) const
{
	return 0;
}

int NoGroupsProxyModel::columnCount(const QModelIndex &AParent) const
{
	return 1;
}

bool NoGroupsProxyModel::hasChildren(const QModelIndex &AParent)
{
	return QAbstractProxyModel::hasChildren(AParent);
}

QVariant NoGroupsProxyModel::data(const QModelIndex &AIndex, int ARole) const
{
	QModelIndex sindex = mapToSource(AIndex);
	if (AIndex.isValid() && !sindex.isValid())
		return QVariant();
	return FModel->data(sindex, ARole);
}

bool NoGroupsProxyModel::setData(const QModelIndex &AIndex, const QVariant &AValue, int ARole)
{
	QModelIndex sindex = mapToSource(AIndex);
	if (AIndex.isValid() && !sindex.isValid())
		return false;
	return FModel->setData(sindex, AValue, ARole);
}

QVariant NoGroupsProxyModel::headerData(int ASection, Qt::Orientation AOrientation, int ARole) const
{
	return QAbstractProxyModel::headerData(ASection, AOrientation, ARole);
}

bool NoGroupsProxyModel::setHeaderData(int ASection, Qt::Orientation AOrientation, const QVariant &AValue, int ARole)
{
	return QAbstractProxyModel::setHeaderData(ASection,AOrientation,AValue,ARole);
}

QStringList NoGroupsProxyModel::mimeTypes() const
{
	return FModel->mimeTypes();
}

QMimeData *NoGroupsProxyModel::mimeData(const QModelIndexList &AIndexes) const
{
	QModelIndexList sindexes;
	for (int i = 0; i < AIndexes.count(); ++i)
		sindexes << mapToSource(AIndexes.at(i));
	return FModel->mimeData(sindexes);
}

bool NoGroupsProxyModel::dropMimeData(const QMimeData *AData, Qt::DropAction AAction,int ARow, int AColumn, const QModelIndex &AParent)
{
	return QAbstractProxyModel::dropMimeData(AData,AAction,ARow,AColumn,AParent);
}

Qt::DropActions NoGroupsProxyModel::supportedDropActions() const
{
	return FModel->supportedDropActions();
}

bool NoGroupsProxyModel::insertRows(int ARow, int ACount, const QModelIndex &AParent)
{
	return QAbstractProxyModel::insertRows(ARow,ACount,AParent);
}

bool NoGroupsProxyModel::insertColumns(int AColumn, int ACount, const QModelIndex &AParent)
{
	return QAbstractProxyModel::insertColumns(AColumn,ACount,AParent);
}

bool NoGroupsProxyModel::removeRows(int ARow, int ACount, const QModelIndex &AParent)
{
	return QAbstractProxyModel::removeRows(ARow,ACount,AParent);
}

bool NoGroupsProxyModel::removeColumns(int AColumn, int ACount, const QModelIndex &AParent)
{
	return QAbstractProxyModel::removeColumns(AColumn,ACount,AParent);
}

void NoGroupsProxyModel::fetchMore(const QModelIndex &AParent)
{
	FModel->fetchMore(mapToSource(AParent));
}

bool NoGroupsProxyModel::canFetchMore(const QModelIndex &AParent) const
{
	return FModel->canFetchMore(mapToSource(AParent));
}

Qt::ItemFlags NoGroupsProxyModel::flags(const QModelIndex &AIndex) const
{
	return FModel->flags(mapToSource(AIndex));
}

QModelIndex NoGroupsProxyModel::buddy(const QModelIndex &AIndex) const
{
	QModelIndex sindex = mapToSource(AIndex);
	QModelIndex sbuddy = FModel->buddy(sindex);
	return sindex!=sbuddy ? mapFromSource(sbuddy) : AIndex;
}

QModelIndexList NoGroupsProxyModel::match(const QModelIndex &AStart, int ARole,const QVariant &AValue, int AHits, Qt::MatchFlags AFlags) const
{
	return QAbstractProxyModel::match(AStart,ARole,AValue,AHits,AFlags);
}

QSize NoGroupsProxyModel::span(const QModelIndex &AIndex) const
{
	QModelIndex sindex = mapToSource(AIndex);
	if (AIndex.isValid() && !sindex.isValid())
		return QSize();
	return FModel->span(sindex);
}

void NoGroupsProxyModel::sort(int AColumn, Qt::SortOrder AOrder)
{
	FModel->sort(AColumn,AOrder);
}

QModelIndex NoGroupsProxyModel::mapFromSource(const QModelIndex &ASourceIndex) const
{
	return ASourceIndex;
}

QModelIndex NoGroupsProxyModel::mapToSource(const QModelIndex &AProxyIndex) const
{
	return AProxyIndex;
}

void NoGroupsProxyModel::setSourceModel(QAbstractItemModel *ASourceModel)
{
	beginResetModel();

	if (FModel)
	{
		disconnect(FModel,SIGNAL(modelAboutToBeReset()),this,SLOT(onSourceAboutToBeReset()));
		disconnect(FModel,SIGNAL(modelReset()),this,SLOT(onSourceReset()));
		disconnect(FModel,SIGNAL(layoutAboutToBeChanged()),this,SLOT(onSourceLayoutAboutToBeChanged()));
		disconnect(FModel,SIGNAL(layoutChanged()),this,SLOT(onSourceLayoutChanged()));

		disconnect(FModel,SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),this,
			SLOT(onSourceDataChanged(const QModelIndex &, const QModelIndex &)));
		disconnect(FModel,SIGNAL(headerDataChanged(Qt::Orientation, int, int)),this,
			SLOT(onSourceHeaderDataChanged(Qt::Orientation, int, int)));

		disconnect(FModel,SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),this,
			SLOT(onSourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(rowsInserted(const QModelIndex &, int, int)),this,
			SLOT(onSourceRowsInserted(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),this,
			SLOT(onSourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(rowsRemoved(const QModelIndex &, int, int)),this,
			SLOT(onSourceRowsRemoved(const QModelIndex &, int, int)));

		disconnect(FModel,SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),this,
			SLOT(onSourceColumnsAboutToBeInserted(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(columnsInserted(const QModelIndex &, int, int)),this,
			SLOT(onSourceColumnsInserted(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),this,
			SLOT(onSourceColumnsAboutToBeRemoved(const QModelIndex &, int, int)));
		disconnect(FModel,SIGNAL(columnsRemoved(const QModelIndex &, int, int)),this,
			SLOT(onSourceColumnsRemoved(const QModelIndex &, int, int)));
	}

	QAbstractProxyModel::setSourceModel(ASourceModel);
	FModel = ASourceModel!=NULL ? ASourceModel : staticEmptyModel();

	if (FModel)
	{
		connect(FModel,SIGNAL(modelAboutToBeReset()),SLOT(onSourceAboutToBeReset()));
		connect(FModel,SIGNAL(modelReset()),SLOT(onSourceReset()));
		connect(FModel,SIGNAL(layoutAboutToBeChanged()),SLOT(onSourceLayoutAboutToBeChanged()));
		connect(FModel,SIGNAL(layoutChanged()),SLOT(onSourceLayoutChanged()));
		
		connect(FModel,SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
			SLOT(onSourceDataChanged(const QModelIndex &, const QModelIndex &)));
		connect(FModel,SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
			SLOT(onSourceHeaderDataChanged(Qt::Orientation, int, int)));
		
		connect(FModel,SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
			SLOT(onSourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(rowsInserted(const QModelIndex &, int, int)),
			SLOT(onSourceRowsInserted(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
			SLOT(onSourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
			SLOT(onSourceRowsRemoved(const QModelIndex &, int, int)));

		connect(FModel,SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
			SLOT(onSourceColumnsAboutToBeInserted(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(columnsInserted(const QModelIndex &, int, int)),
			SLOT(onSourceColumnsInserted(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
			SLOT(onSourceColumnsAboutToBeRemoved(const QModelIndex &, int, int)));
		connect(FModel,SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
			SLOT(onSourceColumnsRemoved(const QModelIndex &, int, int)));
	}

	endResetModel();
}

bool NoGroupsProxyModel::isGroupIndex(const QModelIndex &AIndex) const
{
	static const QList<int> GroupIndexes = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK << RIT_GROUP_NOT_IN_ROSTER;
	return GroupIndexes.contains(AIndex.data(RDR_TYPE).toInt());
}

void NoGroupsProxyModel::onSourceAboutToBeReset()
{

}

void NoGroupsProxyModel::onSourceReset()
{

}

void NoGroupsProxyModel::onSourceLayoutAboutToBeChanged()
{

}

void NoGroupsProxyModel::onSourceLayoutChanged()
{

}

void NoGroupsProxyModel::onSourceDataChanged(const QModelIndex &ASourceTopLeft, const QModelIndex &ASourceBottomRight)
{

}

void NoGroupsProxyModel::onSourceHeaderDataChanged(Qt::Orientation AOrientation, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceRowsAboutToBeInserted(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceRowsInserted(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceRowsAboutToBeRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceRowsRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceColumnsAboutToBeInserted(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceColumnsInserted(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceColumnsAboutToBeRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}

void NoGroupsProxyModel::onSourceColumnsRemoved(const QModelIndex &ASourceParent, int AStart, int AEnd)
{

}
