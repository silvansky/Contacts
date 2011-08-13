#ifndef LEGACYACCOUNTOPTIONS_H
#define LEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/gateserviceidentifiers.h>
#include <interfaces/igateways.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/log.h>
#include "ui_legacyaccountoptions.h"

class LegacyAccountOptions :
	public QWidget
{
	Q_OBJECT
public:
	LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~LegacyAccountOptions();
protected:
	void updateState(const IPresenceItem &APresenceItem, bool AEnabled);
protected slots:
	void onStateCheckboxToggled(bool);
	void onChangeButtonClicked(bool);
	void onChangeDialogAccepted();
	void onDeleteButtonClicked(bool);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
	void onDeleteDialogAccepted();
signals:
	void updated();
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
