#include "metaproxymodel.h"

MetaProxyModel::MetaProxyModel(IMetaContacts *AMetaContacts, IRostersView *ARostersView) : QSortFilterProxyModel(AMetaContacts->instance())
{
	FRostersModel = NULL;
	FRostersView = ARostersView;
	FMetaContacts = AMetaContacts;

	FInvalidateTimer.setInterval(0);
	FInvalidateTimer.setSingleShot(true);
	connect(&FInvalidateTimer,SIGNAL(timeout()),SLOT(onInvalidateTimerTimeout()));

	onRostersModelSet(FRostersView->rostersModel());
	connect(FRostersView->instance(),SIGNAL(modelSet(IRostersModel *)),SLOT(onRostersModelSet(IRostersModel *)));

	connect(FMetaContacts->instance(),SIGNAL(metaContactReceived(IMetaRoster *, const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(IMetaRoster *, const IMetaContact &, const IMetaContact &)));
	connect(FMetaContacts->instance(),SIGNAL(metaRosterEnabled(IMetaRoster *, bool)), SLOT(onMetaRosterEnabled(IMetaRoster *, bool)));
}

MetaProxyModel::~MetaProxyModel()
{

}
int MetaProxyModel::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> MetaProxyModel::rosterDataRoles() const
{
	static QList<int> roles = QList<int>() << Qt::DisplayRole;
	return roles;
}

QList<int> MetaProxyModel::rosterDataTypes() const
{
	static QList<int> types = QList<int>() << RIT_METACONTACT;
	return types;
}

QVariant MetaProxyModel::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	switch (AIndex->type())
	{
	case RIT_METACONTACT:
		if (ARole == Qt::DisplayRole)
		{
			QString name = AIndex->data(RDR_NAME).toString();
			if (name.isEmpty())
				name = Jid(AIndex->data(RDR_INDEX_ID).toString()).node();
			data = name;
		}
		break;
	default:
		break;
	}
	return data;
}

bool MetaProxyModel::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

bool MetaProxyModel::filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const
{
	if (sourceModel())
	{
		QModelIndex index = sourceModel()->index(ASourceRow,0,ASourceParent);
		int indexType = index.data(RDR_TYPE).toInt();
		if (indexType==RIT_CONTACT || indexType==RIT_AGENT)
		{
			IMetaRoster *mroster = FMetaContacts->findMetaRoster(index.data(RDR_STREAM_JID).toString());
			return mroster==NULL || !mroster->isEnabled() || !mroster->itemMetaContact(index.data(RDR_BARE_JID).toString()).isValid();
		}
	}
	return true;
}

void MetaProxyModel::onInvalidateTimerTimeout()
{
	invalidateFilter();
}

void MetaProxyModel::onRostersModelSet(IRostersModel *AModel)
{
	FRostersModel = AModel;
	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
		foreach(Jid streamJid, FRostersModel->streams())
		{
			IMetaRoster *mroster = FMetaContacts->findMetaRoster(streamJid);
			if (mroster)
			{
				foreach(Jid metaId, mroster->metaContacts())
				{
					onMetaContactReceived(mroster,mroster->metaContact(metaId),IMetaContact());
				}
			}
		}
	}
}

void MetaProxyModel::onMetaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled)
{
	if (!AEnabled)
	{
		IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
		if (streamIndex)
		{
			QMultiMap<int,QVariant> findData;
			findData.insert(RDR_TYPE,RIT_METACONTACT);
			foreach(IRosterIndex *index, streamIndex->findChild(findData,true))
			{
				FRostersModel->removeRosterIndex(index);
				index->instance()->deleteLater();
			}
		}
	}
	FInvalidateTimer.start();
}

void MetaProxyModel::onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore)
{
	IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
	if (streamIndex)
	{
		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_INDEX_ID,AContact.id.pBare());
		QList<IRosterIndex *> curItemList = streamIndex->findChild(findData,true);
		QList<IRosterIndex *> oldItemList = curItemList;

		if (!AContact.items.isEmpty())
		{
			QSet<QString> curGroups;
			foreach(IRosterIndex *index, curItemList)
				curGroups.insert(index->data(RDR_GROUP).toString());
			
			QSet<QString> itemGroups = AContact.groups;
			if (itemGroups.isEmpty())
				itemGroups += QString::null;

			QSet<QString> newGroups = itemGroups - curGroups;
			QSet<QString> oldGroups = curGroups - itemGroups;

			QString groupDelim = AMetaRoster->roster()->groupDelimiter();
			foreach(QString group, itemGroups)
			{
				int groupType = !group.isEmpty() ? RIT_GROUP : RIT_GROUP_BLANK;
				QString groupName = !group.isEmpty() ? group : FRostersModel->blankGroupName();
				IRosterIndex *groupIndex = FRostersModel->createGroup(groupName,groupDelim,groupType,streamIndex);

				IRosterIndex *groupItemIndex = NULL;
				if (newGroups.contains(group) && !oldGroups.isEmpty())
				{
					IRosterIndex *oldGroupIndex;
					QString oldGroup = oldGroups.values().value(0);
					if (!oldGroup.isEmpty())
						oldGroupIndex = FRostersModel->findGroup(oldGroup,groupDelim,RIT_GROUP,streamIndex);
					else
						oldGroupIndex = FRostersModel->findGroup(FRostersModel->blankGroupName(),groupDelim,RIT_GROUP_BLANK,streamIndex);
					
					groupItemIndex = oldGroupIndex!=NULL ? oldGroupIndex->findChild(findData).value(0) : NULL;
					if (groupItemIndex)
					{
						groupItemIndex->setData(RDR_GROUP,group);
						groupItemIndex->setParentIndex(groupIndex);
					}

					oldGroups -= oldGroup;
				}
				else
				{
					groupItemIndex = groupIndex->findChild(findData).value(0);
				}

				if (groupItemIndex == NULL)
				{
					groupItemIndex = FRostersModel->createRosterIndex(RIT_METACONTACT,AContact.id.pBare(),groupIndex);
					groupItemIndex->setData(RDR_GROUP,group);
					FRostersModel->insertRosterIndex(groupItemIndex,groupIndex);
				}
				groupItemIndex->setData(RDR_NAME,AContact.name);

				oldItemList.removeAll(groupItemIndex);
			}
		}

		foreach(IRosterIndex *index, oldItemList)
		{
			FRostersModel->removeRosterIndex(index);
			index->instance()->deleteLater();
			FInvalidateTimer.start();
		}

		if (AContact.items != ABefore.items)
			FInvalidateTimer.start();
	}
}
