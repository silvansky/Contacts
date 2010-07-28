#ifndef LEGACYACCOUNTOPTIONS_H
#define LEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <definations/resources.h>
#include <interfaces/igateways.h>
#include <utils/iconstorage.h>
#include "ui_legacyaccountoptions.h"

class LegacyAccountOptions : 
	public QWidget
{
	Q_OBJECT;
public:
	LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~LegacyAccountOptions();
protected slots:
	void onEnableButtonClicked(bool);
	void onDisableButtonClicked(bool);
	void onChangeLinkActivated(const QString &ALink);
	void onDeleteButtonClicked(bool);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
private:
	Ui::LegacyAccountOptionsClass ui;
private:
	IGateways *FGateways;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // LEGACYACCOUNTOPTIONS_H
