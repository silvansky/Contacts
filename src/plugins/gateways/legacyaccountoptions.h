#ifndef LEGACYACCOUNTOPTIONS_H
#define LEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <interfaces/igateways.h>
#include "ui_legacyaccountoptions.h"

class LegacyAccountOptions : 
	public QWidget
{
	Q_OBJECT;
public:
	LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~LegacyAccountOptions();
private:
	Ui::LegacyAccountOptionsClass ui;
private:
	IGateways *FGateways;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // LEGACYACCOUNTOPTIONS_H
