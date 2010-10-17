#include "rosterindex.h"

#include <QSet>
#include <QTimer>

RosterIndex::RosterIndex(int AType, const QString &AId)
{
	FParentIndex = NULL;
	FData.insert(RDR_TYPE,AType);
	FData.insert(RDR_INDEX_ID,AId);
	FFlags = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	FRemoveOnLastChildRemoved = true;
	FRemoveChildsOnRemoved = true;
	FDestroyOnParentRemoved = true;
	FBlokedSetParentIndex = false;
}

RosterIndex::~RosterIndex()
{
	setParentIndex(NULL);
	emit indexDestroyed(this);
}

int RosterIndex::type() const
{
	return data(RDR_TYPE).toInt();
}

QString RosterIndex::id() const
{
	return data(RDR_INDEX_ID).toString();
}

IRosterIndex *RosterIndex::parentIndex() const
{
	return FParentIndex;
}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
	if (FBlokedSetParentIndex || FParentIndex == AIndex)
		return;

	FBlokedSetParentIndex = true;

	if (FParentIndex)
	{
		FParentIndex->removeChild(this);
		setParent(NULL);
	}

	if (AIndex)
	{
		FParentIndex = AIndex;
		FParentIndex->appendChild(this);
		setParent(FParentIndex->instance());
	}
	else
	{
		FParentIndex = NULL;
		if (FDestroyOnParentRemoved)
			QTimer::singleShot(0,this,SLOT(onDestroyByParentRemoved()));
	}

	FBlokedSetParentIndex = false;
}

int RosterIndex::row() const
{
	return FParentIndex ? FParentIndex->childRow(this) : -1;
}

void RosterIndex::appendChild(IRosterIndex *AIndex)
{
	if (AIndex && !FChilds.contains(AIndex))
	{
		emit childAboutToBeInserted(AIndex);
		FChilds.append(AIndex);
		AIndex->setParentIndex(this);
		emit childInserted(AIndex);
	}
}

IRosterIndex * RosterIndex::child(int ARow) const
{
	return FChilds.value(ARow,NULL);
}

int RosterIndex::childRow(const IRosterIndex *AIndex) const
{
	return FChilds.indexOf((IRosterIndex * const)AIndex);
}

int RosterIndex::childCount() const
{
	return FChilds.count();
}

bool RosterIndex::removeChild(IRosterIndex *AIndex)
{
	if (FChilds.contains(AIndex))
	{
		if (AIndex->removeChildsOnRemoved())
			AIndex->removeAllChilds();

		emit childAboutToBeRemoved(AIndex);
		FChilds.removeAt(FChilds.indexOf(AIndex));
		AIndex->setParentIndex(NULL);
		emit childRemoved(AIndex);

		if (FRemoveOnLastChildRemoved && FChilds.isEmpty())
			QTimer::singleShot(0,this,SLOT(onRemoveByLastChildRemoved()));

		return true;
	}
	return false;
}

void RosterIndex::removeAllChilds()
{
	while (FChilds.count()>0)
		removeChild(FChilds.value(0));
}

Qt::ItemFlags RosterIndex::flags() const
{
	return FFlags;
}

void RosterIndex::setFlags( const Qt::ItemFlags &AFlags )
{
	FFlags = AFlags;
}

void RosterIndex::insertDataHolder(IRosterDataHolder *ADataHolder)
{
	connect(ADataHolder->instance(),SIGNAL(rosterDataChanged(IRosterIndex *, int)), SLOT(onDataHolderChanged(IRosterIndex *, int)));

	foreach(int role, ADataHolder->rosterDataRoles())
	{
		FDataHolders[role].insert(ADataHolder->rosterDataOrder(),ADataHolder);
		emit dataChanged(this,role);
	}
	emit dataHolderInserted(ADataHolder);
}

void RosterIndex::removeDataHolder(IRosterDataHolder *ADataHolder)
{
	disconnect(ADataHolder->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
	           this,SLOT(onDataHolderChanged(IRosterIndex *, int)));

	foreach(int role, ADataHolder->rosterDataRoles())
	{
		FDataHolders[role].remove(ADataHolder->rosterDataOrder(),ADataHolder);
		if (FDataHolders[role].isEmpty())
			FDataHolders.remove(role);
		dataChanged(this,role);
	}
	emit dataHolderRemoved(ADataHolder);
}

QVariant RosterIndex::data(int ARole) const
{
	QVariant roleData;
	QList<IRosterDataHolder *> dataHolders = FDataHolders.value(ARole).values();
	for (int i=0; !roleData.isValid() && i<dataHolders.count(); i++)
		roleData = dataHolders.at(i)->rosterData(this,ARole);
	return !roleData.isValid() ? FData.value(ARole) : roleData;
}

QMap<int,QVariant> RosterIndex::data() const
{
	QMap<int,QVariant> values;
	foreach(int role, FDataHolders.keys())
	{
		QList<IRosterDataHolder *> dataHolders = FDataHolders.value(role).values();
		for (int i=0; i<dataHolders.count(); i++)
		{
			QVariant roleData = dataHolders.at(i)->rosterData(this,role);
			if (roleData.isValid())
			{
				values.insert(role,roleData);
				break;
			}
		}
	}
	foreach(int role, FData.keys())
	{
		if (!values.contains(role))
			values.insert(role, FData.value(role));
	}
	return values;
}

void RosterIndex::setData(int ARole, const QVariant &AData)
{
	bool dataSeted = false;
	QList<IRosterDataHolder *> dataHolders = FDataHolders.value(ARole).values();
	for (int i=0; !dataSeted && i<dataHolders.count(); i++)
		dataSeted = dataHolders.value(i)->setRosterData(this,ARole,AData);

	if (!dataSeted)
	{
		if (AData.isValid())
			FData.insert(ARole,AData);
		else
			FData.remove(ARole);
	}

	emit dataChanged(this, ARole);
}

QList<IRosterIndex *> RosterIndex::findChild(const QMultiHash<int, QVariant> AFindData, bool ASearchInChilds) const
{
	QList<IRosterIndex *> indexes;
	foreach (IRosterIndex *index, FChilds)
	{
		bool acceptable = true;
		QList<int> findRoles = AFindData.keys();
		for (int i=0; acceptable && i<findRoles.count(); i++)
		{
			int role = findRoles.at(i);
			if (role == RDR_ANY_ROLE)
			{
				bool found = false;
				QList<QVariant> indexValues = index->data().values();
				QList<QVariant> findValues = AFindData.values(role);
				for (int j=0; !found && j<indexValues.count(); j++)
					found = findValues.contains(indexValues.at(j));
				acceptable = found;
			}
			else if (role==RDR_TYPE && AFindData.values(role).contains(RIT_ANY_TYPE))
				acceptable = true;
			else
				acceptable = AFindData.values(role).contains(index->data(role));
		}
		if (acceptable)
			indexes.append(index);
		if (ASearchInChilds)
			indexes += index->findChild(AFindData,ASearchInChilds);
	}
	return indexes;
}

bool RosterIndex::removeOnLastChildRemoved() const
{
	return FRemoveOnLastChildRemoved;
}

void RosterIndex::setRemoveOnLastChildRemoved(bool ARemove)
{
	FRemoveOnLastChildRemoved = ARemove;
}

bool RosterIndex::removeChildsOnRemoved() const
{
	return FRemoveChildsOnRemoved;
}

void RosterIndex::setRemoveChildsOnRemoved(bool ARemove)
{
	FRemoveChildsOnRemoved = ARemove;
}

bool RosterIndex::destroyOnParentRemoved() const
{
	return FDestroyOnParentRemoved;
}

void RosterIndex::setDestroyOnParentRemoved(bool ADestroy)
{
	FDestroyOnParentRemoved = ADestroy;
}

void RosterIndex::onDataHolderChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == this || AIndex == NULL)
		emit dataChanged(this, ARole);
}

void RosterIndex::onRemoveByLastChildRemoved()
{
	if (FChilds.isEmpty())
		setParentIndex(NULL);
}

void RosterIndex::onDestroyByParentRemoved()
{
	if (!FParentIndex)
		deleteLater();
}
