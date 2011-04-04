#ifndef ADDLEGACYACCOUNTDIALOG_H
#define ADDLEGACYACCOUNTDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <interfaces/igateways.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iregistraton.h>
#include <interfaces/irosterchanger.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include "ui_addlegacyaccountdialog.h"

class AddLegacyAccountDialog :
			public QDialog
{
	Q_OBJECT
public:
	AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent=NULL);
	~AddLegacyAccountDialog();
protected:
	virtual void showEvent(QShowEvent *AEvent);
protected:
	void initialize();
	void abort(const QString &AMessage);
	void setError(const QString &AMessage);
	void setWaitMode(bool AWait, const QString &AMessage = QString::null);
protected slots:
	void onAdjustDialogSize();
	void onLineEditTextChanged(const QString &AText);
	void onShowPasswordStateChanged(int AState);
	void onDialogButtonClicked(QAbstractButton *AButton);
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
	IGateServiceLabel FGateLabel;
	IGateServiceLogin FGateLogin;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // ADDLEGACYACCOUNTDIALOG_H
