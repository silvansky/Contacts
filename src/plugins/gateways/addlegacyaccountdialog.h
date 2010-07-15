#ifndef ADDLEGACYACCOUNTDIALOG_H
#define ADDLEGACYACCOUNTDIALOG_H

#include <QDialog>
#include <interfaces/igateways.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iregistraton.h>
#include "ui_addlegacyaccountdialog.h"

class AddLegacyAccountDialog : 
			public QDialog
{
	Q_OBJECT;
public:
	AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent=NULL);
	~AddLegacyAccountDialog();
protected:
	virtual void showEvent(QShowEvent *AEvent);
protected:
	void initialize();
	void abort(const QString &AMessage);
protected slots:
	void onAdjustDialogSize();
	void onLineEditTextChanged(const QString &AText);
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterSuccess(const QString &AId);
	void onRegisterError(const QString &AId, const QString &AError);
private:
	Ui::AddLegacyAccountDialogClass ui;
private:
	IGateways *FGateways;
	IRegistration *FRegistration;
private:
	QString FRegisterId;
	IGateRegisterLogin FGateLogin;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // ADDLEGACYACCOUNTDIALOG_H
