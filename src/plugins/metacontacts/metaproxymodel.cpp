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
	connect(FRostersView->instance(),SIGNAL(notifyInserted(int)),SLOT(onRostersNotifyInserted(int)));
	connect(FRostersView->instance(),SIGNAL(notifyRemoved(int)),SLOT(onRostersNotifyRemoved(int)));

	connect(FMetaContacts->instance(),SIGNAL(metaAvatarChanged(IMetaRoster *, const QString &)),
		SLOT(onMetaAvatarChanged(IMetaRoster *, const QString &)));
	connect(FMetaContacts->instance(),SIGNAL(metaPresenceChanged(IMetaRoster *, const QString &)),
		SLOT(onMetaPresenceChanged(IMetaRoster *, const QString &)));
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
			return mroster==NULL || !mroster->isEnabled() || mroster->itemMetaContact(index.data(RDR_BARE_JID).toString()).isEmpty();
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
				foreach(QString metaId, mroster->metaContacts())
				{
					onMetaContactReceived(mroster,mroster->metaContact(metaId),IMetaContact());
				}
			}
		}
	}
}

void MetaProxyModel::onRostersNotifyInserted(int ANotifyId)
{
	QSet<IRosterIndex *> metaIndexes;
	foreach(IRosterIndex *index, FRostersView->notifyIndexes(ANotifyId))
	{
		int indexType = index->type();
		if (indexType==RIT_CONTACT || indexType==RIT_AGENT)
		{
			IMetaRoster *mroster = FMetaContacts->findMetaRoster(index->data(RDR_STREAM_JID).toString());
			if (mroster && mroster->isEnabled())
			{
				IRosterIndex *streamIndex = FRostersModel->streamRoot(mroster->streamJid());
				QString metaId = mroster->itemMetaContact(index->data(RDR_BARE_JID).toString());
				if (streamIndex && !metaId.isEmpty())
				{
					QMultiMap<int, QVariant> findData;
					findData.insert(RDR_TYPE,RIT_METACONTACT);
					findData.insert(RDR_INDEX_ID,metaId);
					metaIndexes += streamIndex->findChild(findData,true).toSet();
				}
			}
		}
	}
	if (!metaIndexes.isEmpty())
	{
		int notifyId = FRostersView->insertNotify(FRostersView->notifyById(ANotifyId),metaIndexes.toList());
		FIndexNotifies.insert(ANotifyId,notifyId);
	}
}

void MetaProxyModel::onRostersNotifyRemoved(int ANotifyId)
{
	if (FIndexNotifies.contains(ANotifyId))
	{
		FRostersView->removeNotify(FIndexNotifies.take(ANotifyId));
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

void MetaProxyModel::onMetaAvatarChanged(IMetaRoster *AMetaRoster, const QString &AMetaId)
{
	IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
	if (streamIndex)
	{
		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_INDEX_ID,AMetaId);

		QString hash = AMetaRoster->metaAvatarHash(AMetaId);
		QImage avatar = AMetaRoster->metaAvatarImage(AMetaId).scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation);
		foreach(IRosterIndex *index, streamIndex->findChild(findData,true))
		{
			index->setData(RDR_AVATAR_HASH,hash);
			index->setData(RDR_AVATAR_IMAGE,avatar);
		}
	}
}

void MetaProxyModel::onMetaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId)
{
	IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
	if (streamIndex)
	{
		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_INDEX_ID,AMetaId);
		IPresenceItem pitem = AMetaRoster->metaPresence(AMetaId);
		foreach(IRosterIndex *index, streamIndex->findChild(findData,true))
		{
			index->setData(RDR_SHOW,pitem.show);
			index->setData(RDR_STATUS,pitem.status);
			index->setData(RDR_PRIORITY,pitem.priority);
		}
	}
}

void MetaProxyModel::onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore)
{
	IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
	if (streamIndex)
	{
		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_INDEX_ID,AContact.id);
		QList<IRosterIndex *> curItemList = streamIndex->findChild(findData,true);
		QList<IRosterIndex *> oldItemList = curItemList;

		bool createdNewIndexes = false;
		if (!AContact.items.isEmpty())
		{
			QStringList contactItems;
			foreach(Jid itemJid, AContact.items)
				contactItems.append(itemJid.pBare());

			QSet<QString> curGroups;
			foreach(IRosterIndex *index, curItemList)
				curGroups.insert(index->data(RDR_GROUP).toString());

			int groupType;
			QString groupName;
			QSet<QString> itemGroups;
			if (AContact.items.count()==1 && AContact.items.toList().first().node().isEmpty())
			{
				groupType = RIT_GROUP_AGENTS;
				groupName = FRostersModel->agentsGroupName();
				itemGroups += QString::null;
			}
			else if (AContact.groups.isEmpty())
			{
				groupType = RIT_GROUP_BLANK;
				groupName = FRostersModel->blankGroupName();
				itemGroups += QString::null;
			}
			else
			{
				groupType = RIT_GROUP;
				groupName = FRostersModel->blankGroupName();
				itemGroups = AContact.groups;
			}

			QSet<QString> newGroups = itemGroups - curGroups;
			QSet<QString> oldGroups = curGroups - itemGroups;

			QString groupDelim = AMetaRoster->roster()->groupDelimiter();
			foreach(QString group, itemGroups)
			{
				IRosterIndex *groupIndex = FRostersModel->createGroup(!group.isEmpty() ? group : groupName,groupDelim,groupType,streamIndex);

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
					createdNewIndexes = true;
					groupItemIndex = FRostersModel->createRosterIndex(RIT_METACONTACT,AContact.id,groupIndex);
					groupItemIndex->setData(RDR_GROUP,group);
					groupItemIndex->setData(RDR_NAME,FMetaContacts->metaContactName(AContact));
					groupItemIndex->setData(RDR_METACONTACT_ITEMS,contactItems);
					FRostersModel->insertRosterIndex(groupItemIndex,groupIndex);
				}
				else
				{
					groupItemIndex->setData(RDR_NAME,FMetaContacts->metaContactName(AContact));
					groupItemIndex->setData(RDR_METACONTACT_ITEMS,contactItems);
				}

				oldItemList.removeAll(groupItemIndex);
			}
		}

		foreach(IRosterIndex *index, oldItemList)
		{
			FInvalidateTimer.start();
			FRostersModel->removeRosterIndex(index);
			index->instance()->deleteLater();
		}

		if (createdNewIndexes || AContact.items!=ABefore.items)
		{
			FInvalidateTimer.start();
			onMetaAvatarChanged(AMetaRoster,AContact.id);
			onMetaPresenceChanged(AMetaRoster,AContact.id);
		}
	}
}
