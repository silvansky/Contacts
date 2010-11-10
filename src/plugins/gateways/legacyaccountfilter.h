#ifndef LEGACYACCOUNTFILTER_H
#define LEGACYACCOUNTFILTER_H

#include <QSortFilterProxyModel>
#include <definitions/rosterindextyperole.h>
#include <interfaces/igateways.h>
#include <interfaces/iservicediscovery.h>

class LegacyAccountFilter : 
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	LegacyAccountFilter(IGateways *AGateways, QObject *AParent);
	~LegacyAccountFilter();
protected:
	virtual bool filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const;
protected slots:
	void onStreamServicesChanged(const Jid &AStreamJid);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
private:
	IGateways *FGateways;
private:
	QMap<Jid, QSet<Jid> > FStreamGates;
	QMap<Jid, QSet<Jid> > FEnabledGates;
};

#endif // LEGACYACCOUNTFILTER_H
