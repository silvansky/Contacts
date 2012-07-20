#include "legacyaccountfilter.h"

LegacyAccountFilter::LegacyAccountFilter(IGateways *AGateways, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FGateways = AGateways;
	//connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),
	//	SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	//connect(FGateways->instance(),SIGNAL(streamServicesChanged(const Jid &)),SLOT(onStreamServicesChanged(const Jid &)));
}

LegacyAccountFilter::~LegacyAccountFilter()
{

}

bool LegacyAccountFilter::filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const
{
	if (sourceModel())
	{
		QModelIndex index = sourceModel()->index(ASourceRow,0,ASourceParent);
		int indexType = index.data(RDR_TYPE).toInt();
		if (indexType == RIT_CONTACT)
		{
			Jid contactJid = index.data(RDR_PREP_BARE_JID).toString();
			return !contactJid.node().isEmpty();
		}
		else if (indexType == RIT_METACONTACT)
		{
			foreach(Jid itemJid, index.data(RDR_METACONTACT_ITEMS).toStringList())
				if (!itemJid.node().isEmpty())
					return true;
			return false;
		}
		else if (indexType == RIT_AGENT)
		{
			return false;
		}
	}
	return true;
}

void LegacyAccountFilter::onStreamServicesChanged(const Jid &AStreamJid)
{
	IDiscoIdentity identity;
	identity.category = "gateway";

	QSet<Jid> oldServices = FStreamGates.value(AStreamJid.bare());
	QSet<Jid> newServices = FGateways->streamServices(AStreamJid,identity).toSet();
	if (newServices != oldServices)
	{
		FStreamGates.insert(AStreamJid.bare(),newServices);
		invalidateFilter();
		reset();
	}
}

void LegacyAccountFilter::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	QSet<Jid> &services = FEnabledGates[AStreamJid.bare()];
	if (AEnabled && !services.contains(AServiceJid))
	{
		services += AServiceJid;
		invalidateFilter();
		reset();
	}
	else if (!AEnabled && services.contains(AServiceJid))
	{
		services -= AServiceJid;
		invalidateFilter();
		reset();
	}
}
