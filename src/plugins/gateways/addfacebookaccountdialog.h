#ifndef ADDFACEBOOKACCOUNTDIALOG_H
#define ADDFACEBOOKACCOUNTDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/igateways.h>
#include <interfaces/iregistraton.h>
#include <utils/stylestorage.h>
#include "ui_addfacebookaccountdialog.h"

class AddFacebookAccountDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	AddFacebookAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~AddFacebookAccountDialog();
protected slots:
	void onWebPageLinkClicked(const QUrl &AUrl);
private:
	Ui::AddFacebookAccountDialogClass ui;
private:
	IGateways *FGateways;
	IRegistration *FRegistration;
private:
	QString FRegisterId;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // ADDFACEBOOKACCOUNTDIALOG_H
