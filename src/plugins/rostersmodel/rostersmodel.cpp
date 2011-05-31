#include "rostersmodel.h"

#include <QTimer>

#define INDEX_CHANGES_FOR_RESET 20

RostersModel::RostersModel()
{
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FAccountManager = NULL;

	FRootIndex = new RosterIndex(RIT_ROOT);
	FRootIndex->setParent(this);
	connect(FRootIndex,SIGNAL(dataChanged(IRosterIndex *, int)),
		SLOT(onIndexDataChanged(IRosterIndex *, int)));
	connect(FRootIndex,SIGNAL(childAboutToBeInserted(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childInserted(IRosterIndex *)),
		SLOT(onIndexChildInserted(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childRemoved(IRosterIndex *)),
		SLOT(onIndexChildRemoved(IRosterIndex *)));
}

RostersModel::~RostersModel()
{

}

void RostersModel::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Model");
	APluginInfo->description = tr("Creates a hierarchical model for display roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://friends.rambler.ru";
}

bool RostersModel::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),
				SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),
				SLOT(onPresenceChanged(IPresence *, int , const QString &, int)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
		}
	}

	return true;
}

bool RostersModel::initObjects()
{
	registerSingleGroup(RIT_GROUP_BLANK,tr("Without Groups"));
	registerSingleGroup(RIT_GROUP_AGENTS,tr("Agents"));
	registerSingleGroup(RIT_GROUP_MY_RESOURCES,tr("My Resources"));
	registerSingleGroup(RIT_GROUP_NOT_IN_ROSTER,tr("Not in Roster"));
	return true;
}

QModelIndex RostersModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
	IRosterIndex *parentIndex = rosterIndexByModelIndex(AParent);

	IRosterIndex *childIndex = parentIndex->child(ARow);
	if (childIndex)
		return createIndex(ARow,AColumn,childIndex);

	return QModelIndex();
}

QModelIndex RostersModel::parent(const QModelIndex &AIndex) const
{
	if (AIndex.isValid())
		return modelIndexByRosterIndex(rosterIndexByModelIndex(AIndex)->parentIndex());
	return QModelIndex();
}

bool RostersModel::hasChildren(const QModelIndex &AParent) const
{
	IRosterIndex *parentIndex = rosterIndexByModelIndex(AParent);
	return parentIndex->childCount() > 0;
}

int RostersModel::rowCount(const QModelIndex &AParent) const
{
	IRosterIndex *parentIndex = rosterIndexByModelIndex(AParent);
	return parentIndex->childCount();
}

int RostersModel::columnCount(const QModelIndex &AParent) const
{
	Q_UNUSED(AParent);
	return 1;
}

Qt::ItemFlags RostersModel::flags(const QModelIndex &AIndex) const
{
	IRosterIndex *rosterIndex = rosterIndexByModelIndex(AIndex);
	return rosterIndex->flags();
}

QVariant RostersModel::data(const QModelIndex &AIndex, int ARole) const
{
	IRosterIndex *index = rosterIndexByModelIndex(AIndex);
	return index->data(ARole);
}

QMap<int, QVariant> RostersModel::itemData(const QModelIndex &AIndex) const
{
	IRosterIndex *index = rosterIndexByModelIndex(AIndex);
	return index->data();
}

IRosterIndex *RostersModel::addStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
	if (!streamIndex)
	{
		IRoster *roster = FRosterPlugin ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		IPresence *presence = FPresencePlugin ? FPresencePlugin->getPresence(AStreamJid) : NULL;
		IAccount *account = FAccountManager ? FAccountManager->accountByStream(AStreamJid) : NULL;

		if (roster || presence)
		{
			IRosterIndex *streamIndex = createRosterIndex(RIT_STREAM_ROOT,FRootIndex);
			streamIndex->setRemoveOnLastChildRemoved(false);
			streamIndex->setData(RDR_STREAM_JID,AStreamJid.pFull());
			streamIndex->setData(RDR_FULL_JID,AStreamJid.full());
			streamIndex->setData(RDR_PREP_FULL_JID,AStreamJid.pFull());
			streamIndex->setData(RDR_PREP_BARE_JID,AStreamJid.pBare());

			if (presence)
			{
				streamIndex->setData(RDR_SHOW, presence->show());
				streamIndex->setData(RDR_STATUS,presence->status());
			}
			if (account)
			{
				streamIndex->setData(RDR_NAME,account->name());
				connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
			}

			FStreamsRoot.insert(AStreamJid,streamIndex);
			insertRosterIndex(streamIndex,FRootIndex);
			emit streamAdded(AStreamJid);

			if (roster)
			{
				IRosterItem empty;
				foreach(IRosterItem item, roster->rosterItems()) {
					onRosterItemReceived(roster,item,empty); }
			}
		}
	}
	return streamIndex;
}

QList<Jid> RostersModel::streams() const
{
	return FStreamsRoot.keys();
}

void RostersModel::removeStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamsRoot.take(AStreamJid);
	if (streamIndex)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
		if (account)
		{
			connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),this,SLOT(onAccountOptionsChanged(const OptionsNode &)));
		}
		removeRosterIndex(streamIndex);
		emit streamRemoved(AStreamJid);
	}
}

IRosterIndex *RostersModel::rootIndex() const
{
	return FRootIndex;
}

IRosterIndex *RostersModel::streamRoot(const Jid &AStreamJid) const
{
	return FStreamsRoot.value(AStreamJid);
}

IRosterIndex *RostersModel::createRosterIndex(int AType, IRosterIndex *AParent)
{
	static const struct { int type; int order; }	DefTypeOrders[] = {
		{RIT_STREAM_ROOT,         RITO_STREAM_ROOT}, 
		{RIT_GROUP_BLANK,         RITO_GROUP_BLANK},
		{RIT_GROUP,               RITO_GROUP}, 
		{RIT_GROUP_NOT_IN_ROSTER, RITO_GROUP_NOT_IN_ROSTER}, 
		{RIT_GROUP_MY_RESOURCES,  RITO_GROUP_MY_RESOURCES}, 
		{RIT_GROUP_AGENTS,        RITO_GROUP_AGENTS}, 
		{RIT_CONTACT,             RITO_CONTACT}, 
		{RIT_MY_RESOURCE,         RITO_MY_RESOURCE},
		{RIT_AGENT,               RITO_AGENT},
		{-1,                      -1}
	};

	IRosterIndex *index = new RosterIndex(AType);
	connect(index->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onIndexDestroyed(IRosterIndex *)));

	if (AParent)
		index->setData(RDR_STREAM_JID,AParent->data(RDR_STREAM_JID));

	for (int i=0; DefTypeOrders[i].type>=0; i++)
	{
		if (AType == DefTypeOrders[i].type)
		{
			index->setData(RDR_TYPE_ORDER, DefTypeOrders[i].order);
			break;
		}
	}

	emit indexCreated(index,AParent);
	insertDefaultDataHolders(index);
	return index;
}

IRosterIndex *RostersModel::findGroup(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const
{
	QMultiMap<int,QVariant> findData;
	findData.insert(RDR_TYPE,AType);

	/*
	bool recursive = true;
	if (!FSingleGroups.contains(AType))
	{
		QString parentGroup = AParent->data(RDR_GROUP).toString();
		QString group = AGroup.isEmpty() ? singleGroupName(RIT_GROUP_BLANK) : AGroup;
		QString fullGroup = parentGroup.isEmpty() ? group : parentGroup+AGroupDelim+group;
		recursive = group.contains(AGroupDelim);
		findData.insert(RDR_GROUP,fullGroup);
	}
	IRosterIndex *index = AParent->findChilds(findData,recursive).value(0);
	*/

	QString groupPath = AGroup.isEmpty() ? singleGroupName(AType) : AGroup;
	QList<QString> groupTree = groupPath.split(AGroupDelim,QString::SkipEmptyParts);

	IRosterIndex *groupIndex = AParent;
	do 
	{
		findData.replace(RDR_NAME,groupTree.takeFirst());
		groupIndex = groupIndex->findChilds(findData).value(0);
	} while (groupIndex && !groupTree.isEmpty());

	return groupIndex;
}

IRosterIndex *RostersModel::createGroup(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent)
{
	IRosterIndex *groupIndex = findGroup(AType, AGroup, AGroupDelim, AParent);
	if (!groupIndex)
	{
		QString groupPath = AGroup.isEmpty() ? singleGroupName(AType) : AGroup;
		QList<QString> groupTree = groupPath.split(AGroupDelim,QString::SkipEmptyParts);

		int i = 0;
		groupIndex = AParent;
		IRosterIndex *childGroupIndex = groupIndex;
		QString group = AParent->data(RDR_GROUP).toString();
		while (childGroupIndex && i<groupTree.count())
		{
			if (group.isEmpty())
				group = groupTree.at(i);
			else
				group += AGroupDelim + groupTree.at(i);

			childGroupIndex = findGroup(AType, groupTree.at(i), AGroupDelim, groupIndex);
			if (childGroupIndex)
			{
				groupIndex = childGroupIndex;
				i++;
			}
		}

		while (i < groupTree.count())
		{
			childGroupIndex = createRosterIndex(AType, groupIndex);
			childGroupIndex->setData(RDR_GROUP, !FSingleGroups.contains(AType) ? group : QVariant(QString("")));
			childGroupIndex->setData(RDR_NAME, groupTree.at(i));
			insertRosterIndex(childGroupIndex, groupIndex);
			groupIndex = childGroupIndex;
			group += AGroupDelim + groupTree.value(++i);
		}
	}
	return groupIndex;
}

void RostersModel::insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent)
{
	if (AIndex)
		AIndex->setParentIndex(AParent);
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex)
{
	if (AIndex)
		AIndex->setParentIndex(NULL);
}

QList<IRosterIndex *> RostersModel::getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate)
{
	QList<IRosterIndex *> indexList;
	IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
	if (streamIndex)
	{
		int type = RIT_CONTACT;
		if (AContactJid.node().isEmpty())
			type = RIT_AGENT;
		else if (AContactJid && AStreamJid)
			type = RIT_MY_RESOURCE;

		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE, type);
		if (AContactJid.resource().isEmpty())
			findData.insert(RDR_PREP_BARE_JID, AContactJid.pBare());
		else
			findData.insert(RDR_PREP_FULL_JID, AContactJid.pFull());
		indexList = streamIndex->findChilds(findData,true);

		if (indexList.isEmpty() && !AContactJid.resource().isEmpty())
		{
			findData.insert(RDR_PREP_FULL_JID, AContactJid.pBare());
			indexList = streamIndex->findChilds(findData,true);
		}

		if (ACreate && indexList.isEmpty())
		{
			IRosterIndex *groupIndex;
			if (type == RIT_MY_RESOURCE)
				groupIndex = createGroup(RIT_GROUP_MY_RESOURCES,QString::null,"::",streamIndex);
			else
				groupIndex = createGroup(RIT_GROUP_NOT_IN_ROSTER,QString::null,"::",streamIndex);

			IRosterIndex *index = createRosterIndex(type,groupIndex);
			index->setData(RDR_FULL_JID,AContactJid.full());
			index->setData(RDR_PREP_FULL_JID,AContactJid.pFull());
			index->setData(RDR_PREP_BARE_JID,AContactJid.pBare());
			index->setData(RDR_GROUP,groupIndex->data(RDR_GROUP));
			insertRosterIndex(index,groupIndex);
			indexList.append(index);
		}
	}
	return indexList;
}

QModelIndex RostersModel::modelIndexByRosterIndex(IRosterIndex *AIndex) const
{
	return AIndex!=NULL && AIndex!=FRootIndex ? createIndex(AIndex->row(),0,AIndex) : QModelIndex();
}

IRosterIndex *RostersModel::rosterIndexByModelIndex(const QModelIndex &AIndex) const
{
	return AIndex.isValid() ? reinterpret_cast<IRosterIndex *>(AIndex.internalPointer()) : FRootIndex;
}

QString RostersModel::singleGroupName(int AType) const
{
	return FSingleGroups.contains(AType) ? FSingleGroups.value(AType) : FSingleGroups.value(RIT_GROUP_BLANK);
}

void RostersModel::registerSingleGroup(int AType, const QString &AName)
{
	if (!FSingleGroups.contains(AType))
	{
		FSingleGroups.insert(AType,AName);
	}
}

void RostersModel::insertDefaultDataHolder(IRosterDataHolder *ADataHolder)
{
	if (ADataHolder && !FDataHolders.contains(ADataHolder))
	{
		QMultiMap<int,QVariant> findData;
		foreach(int type, ADataHolder->rosterDataTypes())
			findData.insertMulti(RDR_TYPE,type);

		foreach(IRosterIndex *index, FRootIndex->findChilds(findData,true))
			index->insertDataHolder(ADataHolder);

		FDataHolders.append(ADataHolder);
		emit defaultDataHolderInserted(ADataHolder);
	}
}

void RostersModel::removeDefaultDataHolder(IRosterDataHolder *ADataHolder)
{
	if (FDataHolders.contains(ADataHolder))
	{
		QMultiMap<int,QVariant> findData;
		foreach(int type, ADataHolder->rosterDataTypes())
			findData.insertMulti(RDR_TYPE,type);

		QList<IRosterIndex *> indexes = FRootIndex->findChilds(findData,true);
		foreach(IRosterIndex *index, indexes)
			index->removeDataHolder(ADataHolder);

		FDataHolders.removeAll(ADataHolder);
		emit defaultDataHolderRemoved(ADataHolder);
	}
}

void RostersModel::emitDelayedDataChanged(IRosterIndex *AIndex)
{
	FChangedIndexes -= AIndex;

	if (AIndex != FRootIndex)
	{
		QModelIndex modelIndex = modelIndexByRosterIndex(AIndex);
		emit dataChanged(modelIndex,modelIndex);
	}

	QList<IRosterIndex *> childs;
	foreach(IRosterIndex *index, FChangedIndexes)
		if (index->parentIndex() == AIndex)
			childs.append(index);

	foreach(IRosterIndex *index, childs)
		emitDelayedDataChanged(index);
}

void RostersModel::insertDefaultDataHolders(IRosterIndex *AIndex)
{
	foreach(IRosterDataHolder *dataHolder, FDataHolders)
		if (dataHolder->rosterDataTypes().contains(RIT_ANY_TYPE) || dataHolder->rosterDataTypes().contains(AIndex->type()))
			AIndex->insertDataHolder(dataHolder);
}

void RostersModel::onAccountShown(IAccount *AAccount)
{
	if (AAccount->isActive())
		addStream(AAccount->xmppStream()->streamJid());
}

void RostersModel::onAccountHidden(IAccount *AAccount)
{
	if (AAccount->isActive())
		removeStream(AAccount->xmppStream()->streamJid());
}

void RostersModel::onAccountOptionsChanged(const OptionsNode &ANode)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account && account->isActive() && account->optionsNode().childPath(ANode)=="name")
	{
		IRosterIndex *streamIndex = FStreamsRoot.value(account->xmppStream()->streamJid());
		if (streamIndex)
			streamIndex->setData(RDR_NAME,account->name());
	}
}

void RostersModel::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamsRoot.value(ARoster->streamJid());
	if (streamIndex)
	{
		int groupType;
		QString groupDisplay;
		QSet<QString> itemGroups;
		int itemType = !AItem.itemJid.node().isEmpty() ? RIT_CONTACT : RIT_AGENT;

		if (itemType == RIT_AGENT)
		{
			groupType = RIT_GROUP_AGENTS;
			itemGroups += QString::null;
		}
		else if (AItem.groups.isEmpty())
		{
			groupType = RIT_GROUP_BLANK;
			itemGroups += QString::null;
		}
		else
		{
			groupType = RIT_GROUP;
			itemGroups = AItem.groups;
		}

		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_TYPE,itemType);
		findData.insert(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
		QList<IRosterIndex *> curItemList = streamIndex->findChilds(findData,true);

		QList<IRosterIndex *> itemList;
		if (AItem.subscription != SUBSCRIPTION_REMOVE)
		{
			QSet<QString> curGroups;
			foreach(IRosterIndex *index, curItemList)
				curGroups.insert(index->data(RDR_GROUP).toString());

			QSet<QString> newGroups = itemGroups - curGroups;
			QSet<QString> oldGroups = curGroups - itemGroups;

			QString groupDelim = ARoster->groupDelimiter();
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(ARoster->streamJid()) : NULL;
			foreach(QString group,itemGroups)
			{
				IRosterIndex *groupIndex = createGroup(groupType,group,groupDelim,streamIndex);

				QList<IRosterIndex *> groupItemList;
				if (newGroups.contains(group) && !oldGroups.isEmpty())
				{
					IRosterIndex *oldGroupIndex;
					QString oldGroup = oldGroups.values().value(0);
					if (oldGroup.isEmpty())
						oldGroupIndex = findGroup(RIT_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						oldGroupIndex = findGroup(RIT_GROUP,oldGroup,groupDelim,streamIndex);
					if (oldGroupIndex)
					{
						groupItemList = oldGroupIndex->findChilds(findData);
						foreach(IRosterIndex *index,groupItemList)
						{
							index->setData(RDR_GROUP,group);
							index->setParentIndex(groupIndex);
						}
					}
					oldGroups -= group;
				}
				else
				{
					groupItemList = groupIndex->findChilds(findData);
				}

				if (groupItemList.isEmpty())
				{
					int presIndex = 0;
					QList<IPresenceItem> pitems = presence!=NULL ? presence->presenceItems(AItem.itemJid) : QList<IPresenceItem>();
					do
					{
						IRosterIndex *index;
						IPresenceItem pitem = pitems.value(presIndex++);
						if (pitem.isValid)
						{
							index = createRosterIndex(itemType,groupIndex);
							index->setData(RDR_FULL_JID,pitem.itemJid.full());
							index->setData(RDR_PREP_FULL_JID,pitem.itemJid.pFull());
							index->setData(RDR_SHOW,pitem.show);
							index->setData(RDR_STATUS,pitem.status);
							index->setData(RDR_PRIORITY,pitem.priority);
						}
						else
						{
							index = createRosterIndex(itemType,groupIndex);
							index->setData(RDR_FULL_JID,AItem.itemJid.bare());
							index->setData(RDR_PREP_FULL_JID,AItem.itemJid.pBare());
						}

						index->setData(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
						index->setData(RDR_NAME,AItem.name);
						index->setData(RDR_SUBSCRIBTION,AItem.subscription);
						index->setData(RDR_ASK,AItem.ask);
						index->setData(RDR_GROUP,group);

						itemList.append(index);
						insertRosterIndex(index,groupIndex);
					}
					while (presIndex < pitems.count());
				}
				else foreach(IRosterIndex *index,groupItemList)
				{
					index->setData(RDR_NAME,AItem.name);
					index->setData(RDR_SUBSCRIBTION,AItem.subscription);
					index->setData(RDR_ASK,AItem.ask);
					itemList.append(index);
				}
			}
		}

		foreach(IRosterIndex *index, curItemList)
			if (!itemList.contains(index))
				removeRosterIndex(index);
	}
}

void RostersModel::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(ABefore);
	if (streamIndex)
	{
		Jid after = ARoster->streamJid();

		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_STREAM_JID,ABefore.pFull());
		QList<IRosterIndex *> itemList = FRootIndex->findChilds(findData,true);
		foreach(IRosterIndex *index, itemList)
			index->setData(RDR_STREAM_JID,after.pFull());

		streamIndex->setData(RDR_FULL_JID,after.full());
		streamIndex->setData(RDR_PREP_FULL_JID,after.pFull());

		FStreamsRoot.remove(ABefore);
		FStreamsRoot.insert(after,streamIndex);

		emit streamJidChanged(ABefore,after);
	}
}

void RostersModel::onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
	if (streamIndex)
	{
		streamIndex->setData(RDR_SHOW, AShow);
		streamIndex->setData(RDR_STATUS, AStatus);
		if (AShow != IPresence::Offline && AShow != IPresence::Error)
			streamIndex->setData(RDR_PRIORITY, APriority);
		else
			streamIndex->setData(RDR_PRIORITY, QVariant());
	}
}

void RostersModel::onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
	if (streamIndex)
	{
		int itemType = RIT_CONTACT;
		if (AItem.itemJid.node().isEmpty())
			itemType = RIT_AGENT;
		else if (AItem.itemJid && APresence->streamJid())
			itemType = RIT_MY_RESOURCE;

		if (AItem.show == IPresence::Offline)
		{
			QMultiMap<int,QVariant> findData;
			findData.insert(RDR_TYPE,itemType);
			findData.insert(RDR_PREP_FULL_JID,AItem.itemJid.pFull());
			QList<IRosterIndex *> itemList = streamIndex->findChilds(findData, true);
			int pitemsCount = APresence->presenceItems(AItem.itemJid).count();
			foreach (IRosterIndex *index,itemList)
			{
				if (itemType == RIT_MY_RESOURCE || pitemsCount > 1)
				{
					removeRosterIndex(index);
				}
				else
				{
					index->setData(RDR_FULL_JID,AItem.itemJid.bare());
					index->setData(RDR_PREP_FULL_JID,AItem.itemJid.pBare());
					index->setData(RDR_SHOW,AItem.show);
					index->setData(RDR_STATUS,AItem.status);
					index->setData(RDR_PRIORITY,QVariant());
				}
			}
		}
		else if (AItem.show == IPresence::Error)
		{
			QMultiMap<int,QVariant> findData;
			findData.insert(RDR_TYPE,itemType);
			findData.insert(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
			QList<IRosterIndex *> itemList = streamIndex->findChilds(findData,true);
			foreach(IRosterIndex *index,itemList)
			{
				index->setData(RDR_SHOW,AItem.show);
				index->setData(RDR_STATUS,AItem.status);
				index->setData(RDR_PRIORITY,QVariant());
			}
		}
		else
		{
			QMultiMap<int,QVariant> findData;
			findData.insert(RDR_TYPE,itemType);
			findData.insert(RDR_PREP_FULL_JID,AItem.itemJid.pFull());
			QList<IRosterIndex *> itemList = streamIndex->findChilds(findData,true);
			if (itemList.isEmpty())
			{
				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(APresence->streamJid()) : NULL;
				IRosterItem ritem = roster!=NULL ? roster->rosterItem(AItem.itemJid) : IRosterItem();
				QString groupDelim = roster!=NULL ? roster->groupDelimiter() : "::";

				QSet<QString> itemGroups;
				if (ritem.isValid)
				{
					if (!ritem.groups.isEmpty())
						itemGroups = ritem.groups;
					else
						itemGroups.insert(QString::null);
				}
				else if (itemType == RIT_MY_RESOURCE)
				{
					itemGroups.insert(QString::null);
				}

				foreach(QString group,itemGroups)
				{
					IRosterIndex *groupIndex = NULL;
					if (itemType == RIT_MY_RESOURCE)
						groupIndex = createGroup(RIT_GROUP_MY_RESOURCES,QString::null,groupDelim,streamIndex);
					else if (!ritem.isValid)
						groupIndex = createGroup(RIT_GROUP_NOT_IN_ROSTER,QString::null,groupDelim,streamIndex);
					else if (itemType == RIT_AGENT)
						groupIndex = findGroup(RIT_GROUP_AGENTS,QString::null,groupDelim,streamIndex);
					else if (group.isEmpty())
						groupIndex = findGroup(RIT_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						groupIndex = findGroup(RIT_GROUP,group,groupDelim,streamIndex);

					if (groupIndex)
					{
						findData.replace(RDR_PREP_FULL_JID,AItem.itemJid.pBare());
						IRosterIndex *index = groupIndex->findChilds(findData).value(0);
						if (!index)
						{
							index = createRosterIndex(itemType,groupIndex);
							index->setData(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
							index->setData(RDR_GROUP,group);
							if (ritem.isValid)
							{
								index->setData(RDR_NAME,ritem.name);
								index->setData(RDR_SUBSCRIBTION,ritem.subscription);
								index->setData(RDR_ASK,ritem.ask);
							}
						}

						index->setData(RDR_FULL_JID,AItem.itemJid.full());
						index->setData(RDR_PREP_FULL_JID,AItem.itemJid.pFull());
						index->setData(RDR_SHOW,AItem.show);
						index->setData(RDR_STATUS,AItem.status);
						index->setData(RDR_PRIORITY,AItem.priority);
						insertRosterIndex(index,groupIndex);
					}
				}
			}
			else foreach(IRosterIndex *index,itemList)
			{
				index->setData(RDR_SHOW,AItem.show);
				index->setData(RDR_STATUS,AItem.status);
				index->setData(RDR_PRIORITY,AItem.priority);
			}
		}
	}
}

void RostersModel::onIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (FChangedIndexes.isEmpty())
		QTimer::singleShot(0,this,SLOT(onDelayedDataChanged()));
	FChangedIndexes+=AIndex;
	emit indexDataChanged(AIndex, ARole);
}

void RostersModel::onIndexChildAboutToBeInserted(IRosterIndex *AIndex)
{
	emit indexAboutToBeInserted(AIndex);
	beginInsertRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->parentIndex()->childCount(),AIndex->parentIndex()->childCount());
	connect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
		SLOT(onIndexDataChanged(IRosterIndex *, int)));
	connect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
		SLOT(onIndexChildInserted(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
		SLOT(onIndexChildRemoved(IRosterIndex *)));
}

void RostersModel::onIndexChildInserted(IRosterIndex *AIndex)
{
	endInsertRows();
	emit indexInserted(AIndex);
}

void RostersModel::onIndexChildAboutToBeRemoved(IRosterIndex *AIndex)
{
	FChangedIndexes-=AIndex;
	emit indexAboutToBeRemoved(AIndex);
	beginRemoveRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->row(),AIndex->row());
}

void RostersModel::onIndexChildRemoved(IRosterIndex *AIndex)
{
	disconnect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
		   this,SLOT(onIndexDataChanged(IRosterIndex *, int)));
	disconnect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
		   this,SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
		   this,SLOT(onIndexChildInserted(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
		   this,SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
		   this,SLOT(onIndexChildRemoved(IRosterIndex *)));
	endRemoveRows();
	emit indexRemoved(AIndex);
}

void RostersModel::onIndexDestroyed(IRosterIndex *AIndex)
{
	emit indexDestroyed(AIndex);
}

void RostersModel::onDelayedDataChanged()
{
	//Замена сигналов изменения большого числа индексов на сброс модели
	//из-за тормозов в сортировке в QSortFilterProxyModel
	if (FChangedIndexes.count() < INDEX_CHANGES_FOR_RESET)
	{
		//Вызывает dataChanged у всех родителей для поддержки SortFilterProxyModel
		QSet<IRosterIndex *> childIndexes = FChangedIndexes;
		foreach(IRosterIndex *index,childIndexes)
		{
			IRosterIndex *parentIndex = index->parentIndex();
			while (parentIndex && !FChangedIndexes.contains(parentIndex))
			{
				FChangedIndexes+=parentIndex;
				parentIndex = parentIndex->parentIndex();
			}
			QModelIndex modelIndex = modelIndexByRosterIndex(index);
			emit dataChanged(modelIndex,modelIndex);
		}
		emitDelayedDataChanged(FRootIndex);
	}
	else
	{
		reset();
	}

	FChangedIndexes.clear();
}

Q_EXPORT_PLUGIN2(plg_rostersmodel, RostersModel)
