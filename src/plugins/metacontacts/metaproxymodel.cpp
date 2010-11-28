#include "metaproxymodel.h"

MetaProxyModel::MetaProxyModel(IMetaContacts *AMetaContacts, IRostersView *ARostersView) : QSortFilterProxyModel(AMetaContacts->instance())
{
	FRostersModel = NULL;
	FRostersView = ARostersView;
	FMetaContacts = AMetaContacts;

	onRostersModelSet(FRostersView->rostersModel());
	connect(FRostersView->instance(),SIGNAL(modelSet(IRostersModel *)),SLOT(onRostersModelSet(IRostersModel *)));

	connect(FMetaContacts->instance(),SIGNAL(metaContactReceived(IMetaRoster *, const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(IMetaRoster *, const IMetaContact &, const IMetaContact &)));
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

		}
	}
	return true;
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

void MetaProxyModel::onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore)
{
	if (FRostersModel)
	{

	}
}
