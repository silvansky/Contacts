#include "metaproxymodel.h"
#include <definitions/statusicons.h>
#include <definitions/rosterlabelorders.h>

MetaProxyModel::MetaProxyModel(IMetaContacts *AMetaContacts, IRostersView *ARostersView) : QSortFilterProxyModel(AMetaContacts->instance())
{
	FRostersModel = NULL;
	FRostersView = ARostersView;

	if (FRostersView)
	{
		IRostersLabel rlabel;
		rlabel.order = RLO_GATEWAY_ICON;
		rlabel.label = RDR_GATEWAY_ICON;
		FRostersLabel = FRostersView->registerLabel(rlabel);
	}
	else
		FRostersLabel = -1;

	FMetaContacts = AMetaContacts;

	FInvalidateTimer.setInterval(0);
	FInvalidateTimer.setSingleShot(true);
	connect(&FInvalidateTimer,SIGNAL(timeout()),SLOT(onInvalidateTimerTimeout()));

	onRostersModelSet(FRostersView->rostersModel());
	connect(FRostersView->instance(),SIGNAL(modelSet(IRostersModel *)),SLOT(onRostersModelSet(IRostersModel *)));
	connect(FRostersView->instance(),SIGNAL(notifyInserted(int)),SLOT(onRostersNotifyInserted(int)));
	connect(FRostersView->instance(),SIGNAL(notifyActivated(int)),SLOT(onRostersNotifyActivated(int)));
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
	static QList<int> roles = QList<int>() << RDR_FOOTER_TEXT << Qt::DecorationRole << Qt::DisplayRole << RDR_GATEWAY_ICON;
	return roles;
}

QList<int> MetaProxyModel::rosterDataTypes() const
{
	static QList<int> types = QList<int>()
			<< RIT_METACONTACT;
	return types;
}

QVariant MetaProxyModel::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	static bool block = false;

	QVariant data;
	switch (AIndex->type())
	{
	case RIT_METACONTACT:
		if (ARole == Qt::DisplayRole)
		{
			QString name = AIndex->data(RDR_NAME).toString();
			if (name.isEmpty())
				name = Jid(AIndex->data(RDR_METACONTACT_ITEMS).toStringList().value(0)).node();
			data = name;
		}
		else if (!block)
		{
			block = true;
			IMetaRoster *mroster = FMetaContacts->findMetaRoster(AIndex->data(RDR_STREAM_JID).toString());
			IMetaContact contact = mroster->metaContact(AIndex->data(RDR_META_ID).toString());

			if (!mroster->roster()->subscriptionRequests().intersect(contact.items).isEmpty())
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Requests authorization"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
				}
			}
			else if (AIndex->data(RDR_ASK).toString() == SUBSCRIPTION_SUBSCRIBE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Sent an authorization request"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
				}
			}
			else if (AIndex->data(RDR_SUBSCRIBTION).toString() == SUBSCRIPTION_NONE)
			{
				if (ARole == RDR_FOOTER_TEXT)
				{
					QVariantMap footer = AIndex->data(ARole).toMap();
					footer.insert(QString("%1").arg(FTO_ROSTERSVIEW_STATUS,10,10,QLatin1Char('0')),tr("Not authorized"));
					data = footer;
				}
				else if (ARole == Qt::DecorationRole)
				{
					data = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS)->getIcon(STI_NOAUTH);
				}
			}
			block = false;
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

QList<IRosterIndex *> MetaProxyModel::findMetaIndexes(IMetaRoster *AMetaRoster, const QString &AMetaId) const
{
	return FMetaIndexes.value(AMetaRoster).values(AMetaId);
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
			return mroster==NULL || !mroster->isEnabled() || mroster->itemMetaContact(index.data(RDR_PREP_BARE_JID).toString()).isEmpty();
		}
	}
	return true;
}

IRosterIndex *MetaProxyModel::findChildIndex(const QList<IRosterIndex *> &AIndexes, IRosterIndex *AParent) const
{
	foreach(IRosterIndex *index, AIndexes)
		if (index->parentIndex() == AParent)
			return index;
	return NULL;
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
				QString metaId = mroster->itemMetaContact(index->data(RDR_PREP_BARE_JID).toString());
				if (!metaId.isEmpty())
					metaIndexes += findMetaIndexes(mroster,metaId).toSet();
			}
		}
	}
	if (!metaIndexes.isEmpty())
	{
		int notifyId = FRostersView->insertNotify(FRostersView->notifyById(ANotifyId),metaIndexes.toList());
		FIndexNotifies.insert(ANotifyId,notifyId);
	}
}

void MetaProxyModel::onRostersNotifyActivated(int ANotifyId)
{
	static int blockNotifyId = -1;
	if (blockNotifyId != ANotifyId)
	{
		if (FIndexNotifies.contains(ANotifyId))
		{
			blockNotifyId = FIndexNotifies.value(ANotifyId);
			FRostersView->activateNotify(blockNotifyId);
			blockNotifyId = -1;
		}
		else if (FIndexNotifies.values().contains(ANotifyId))
		{
			blockNotifyId = FIndexNotifies.key(ANotifyId);
			FRostersView->activateNotify(blockNotifyId);
			blockNotifyId = -1;
		}
	}
}

void MetaProxyModel::onRostersNotifyRemoved(int ANotifyId)
{
	if (FIndexNotifies.contains(ANotifyId))
	{
		FRostersView->removeNotify(FIndexNotifies.take(ANotifyId));
	}
	else if (FIndexNotifies.values().contains(ANotifyId))
	{
		FRostersView->removeNotify(FIndexNotifies.key(ANotifyId));
	}
}

void MetaProxyModel::onMetaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled)
{
	if (!AEnabled)
	{
		QMultiHash<QString, IRosterIndex *> rosterMetaIndexes = FMetaIndexes[AMetaRoster];
		QMultiHash<QString, IRosterIndex *>::iterator it=rosterMetaIndexes.begin();
		while(it!=rosterMetaIndexes.end())
		{
			IRosterIndex *index = it.value();
			FRostersModel->removeRosterIndex(index);
			index->instance()->deleteLater();
			it = rosterMetaIndexes.erase(it);
		}
		FMetaIndexes.remove(AMetaRoster);
	}
	FInvalidateTimer.start();
}

void MetaProxyModel::onMetaAvatarChanged(IMetaRoster *AMetaRoster, const QString &AMetaId)
{
	QString hash = AMetaRoster->metaAvatarHash(AMetaId);
	QImage originalAvatar = AMetaRoster->metaAvatarImage(AMetaId, false);
	QImage avatar = ImageManager::roundSquared(originalAvatar, 24, 2);
	originalAvatar = AMetaRoster->metaAvatarImage(AMetaId, true);
	if (originalAvatar.isNull())
	{
		originalAvatar = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_AVATAR_EMPTY_MALE, 1);
	}
	QImage largeAvatar = ImageManager::roundSquared(originalAvatar, 36, 2);

	foreach(IRosterIndex *index, findMetaIndexes(AMetaRoster,AMetaId))
	{
		index->setData(RDR_AVATAR_HASH, hash);
		index->setData(RDR_AVATAR_IMAGE, avatar);
		index->setData(RDR_AVATAR_IMAGE_LARGE, largeAvatar);
	}
}

void MetaProxyModel::onMetaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId)
{
	IPresenceItem pitem = AMetaRoster->metaPresenceItem(AMetaId);
	foreach(IRosterIndex *index, findMetaIndexes(AMetaRoster,AMetaId))
	{
		index->setData(RDR_SHOW,pitem.show);
		index->setData(RDR_STATUS,pitem.status);
		index->setData(RDR_PRIORITY,pitem.priority);
	}
}

void MetaProxyModel::onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore)
{
	IRosterIndex *streamIndex = FRostersModel!=NULL ? FRostersModel->streamRoot(AMetaRoster->streamJid()) : NULL;
	if (streamIndex)
	{
		QMultiHash<QString, IRosterIndex *> &rosterMetaIndexes = FMetaIndexes[AMetaRoster];
		QList<IRosterIndex *> curItemList = rosterMetaIndexes.values(AContact.id);
		QList<IRosterIndex *> oldItemList = curItemList;

		bool createdNewIndexes = false;
		if (!AContact.items.isEmpty())
		{
			QStringList contactItems;
			foreach(Jid itemJid, FMetaContacts->itemOrders(AContact.items.toList()).values())
				contactItems.append(itemJid.pBare());

			QSet<QString> curGroups;
			foreach(IRosterIndex *index, curItemList)
				curGroups.insert(index->data(RDR_GROUP).toString());

			int groupType;
			QSet<QString> itemGroups;
			if (AContact.items.count()==1 && AContact.items.toList().first().node().isEmpty())
			{
				groupType = RIT_GROUP_AGENTS;
				itemGroups += QString::null;
			}
			else if (AContact.groups.isEmpty())
			{
				groupType = RIT_GROUP_BLANK;
				itemGroups += QString::null;
			}
			else
			{
				groupType = RIT_GROUP;
				itemGroups = AContact.groups;
			}

			QSet<QString> newGroups = itemGroups - curGroups;
			QSet<QString> oldGroups = curGroups - itemGroups;

			QString groupDelim = AMetaRoster->roster()->groupDelimiter();
			foreach(QString group, itemGroups)
			{
				IRosterIndex *groupIndex = FRostersModel->createGroupIndex(groupType,group,groupDelim,streamIndex);

				IRosterIndex *groupItemIndex = NULL;
				if (newGroups.contains(group) && !oldGroups.isEmpty())
				{
					IRosterIndex *oldGroupIndex;
					QString oldGroup = oldGroups.values().value(0);
					if (!oldGroup.isEmpty())
						oldGroupIndex = FRostersModel->findGroupIndex(RIT_GROUP,oldGroup,groupDelim,streamIndex);
					else
						oldGroupIndex = FRostersModel->findGroupIndex(RIT_GROUP_BLANK,QString::null,groupDelim,streamIndex);

					groupItemIndex = oldGroupIndex!=NULL ? findChildIndex(curItemList,oldGroupIndex) : NULL;
					if (groupItemIndex)
					{
						groupItemIndex->setData(RDR_GROUP,group);
						groupItemIndex->setParentIndex(groupIndex);
					}

					oldGroups -= oldGroup;
				}
				else
				{
					groupItemIndex = findChildIndex(curItemList,groupIndex);
				}

				if (groupItemIndex == NULL)
				{
					createdNewIndexes = true;
					groupItemIndex = FRostersModel->createRosterIndex(RIT_METACONTACT,groupIndex);
					groupItemIndex->setData(RDR_TYPE_ORDER,RITO_METACONTACT);
					groupItemIndex->setData(RDR_GROUP,group);
					groupItemIndex->setData(RDR_META_ID,AContact.id);
					if (FRostersView)
						FRostersView->insertLabel(FRostersLabel, groupItemIndex);
					rosterMetaIndexes.insertMulti(AContact.id,groupItemIndex);
				}

				groupItemIndex->setData(RDR_NAME,FMetaContacts->metaContactName(AContact));
				groupItemIndex->setData(RDR_ASK,AContact.ask);
				groupItemIndex->setData(RDR_SUBSCRIBTION,AContact.subscription);
				groupItemIndex->setData(RDR_METACONTACT_ITEMS,contactItems);
				FRostersModel->insertRosterIndex(groupItemIndex,groupIndex);

				if (AContact.items != ABefore.items)
				{
					if (AContact.items.count() == 1)
					{
						QIcon icon;
						IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(*(AContact.items.constBegin()));
						icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 4)));
						icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 5)), QIcon::Selected);
						groupItemIndex->setData(RDR_GATEWAY_ICON,icon);
					}
					else
					{
						groupItemIndex->setData(RDR_GATEWAY_ICON,QVariant());
					}
				}

				emit rosterDataChanged(groupItemIndex,Qt::DecorationRole);
				emit rosterDataChanged(groupItemIndex,RDR_FOOTER_TEXT);

				oldItemList.removeAll(groupItemIndex);
			}
		}

		foreach(IRosterIndex *index, oldItemList)
		{
			FInvalidateTimer.start();
			FRostersModel->removeRosterIndex(index);
			index->instance()->deleteLater();
			rosterMetaIndexes.remove(AContact.id,index);
		}

		if (!AContact.items.isEmpty() && (createdNewIndexes || AContact.items!=ABefore.items))
		{
			FInvalidateTimer.start();
			onMetaAvatarChanged(AMetaRoster,AContact.id);
			onMetaPresenceChanged(AMetaRoster,AContact.id);
		}
	}
}
