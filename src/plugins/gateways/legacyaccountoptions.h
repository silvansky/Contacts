#ifndef LEGACYACCOUNTOPTIONS_H
#define LEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <definitions/resources.h>
#include <interfaces/igateways.h>
#include <utils/iconstorage.h>
#include "ui_legacyaccountoptions.h"

class LegacyAccountOptions :
	public QWidget
{
	Q_OBJECT
public:
	LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~LegacyAccountOptions();
protected slots:
	void onEnableBoxToggled(bool);
	void onEnableButtonClicked(bool);
	void onDisableButtonClicked(bool);
	void onChangeLinkActivated(const QString &ALink);
	void onChangeButtonClicked();
	void onChangeDialogAccepted();
	void onDeleteButtonClicked(bool);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
private:
	Ui::LegacyAccountOptionsClass ui;
private:
	IGateways *FGateways;
	IGateServiceLabel FGateLabel;
private:
	Jid FStreamJid;
	Jid FServiceJid;
	QString FLoginRequest;
};

#endif // LEGACYACCOUNTOPTIONS_H
