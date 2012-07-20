#ifndef ADDLEGACYACCOUNTDIALOG_H
#define ADDLEGACYACCOUNTDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <interfaces/ipresence.h>
#include <interfaces/igateways.h>
#include <interfaces/iregistraton.h>
#include <utils/log.h>
#include <utils/menu.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/custominputdialog.h>
#include "ui_addlegacyaccountdialog.h"

class AddLegacyAccountDialog :
	public QDialog
{
	Q_OBJECT
public:
	AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, IPresence *APresence, const Jid &AServiceJid, QWidget *AParent=NULL);
	~AddLegacyAccountDialog();
protected:
	virtual void showEvent(QShowEvent *AEvent);
protected:
	void abort(const QString &AMessage);
	void setError(const QString &AMessage);
	void setWaitMode(bool AWait, const QString &AMessage = QString::null);
protected slots:
	void onAdjustDialogSize();
	void onLineEditTextChanged(const QString &AText);
	void onShowPasswordStateChanged(int AState);
	void onDialogButtonClicked(QAbstractButton *AButton);
	void onOkClicked();
	void onCancelClicked();
	void onDomainsMenuActionTriggered();
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterSuccess(const QString &AId);
	void onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage);
private:
	Ui::AddLegacyAccountDialogClass ui;
private:
	IPresence *FPresence;
	IGateways *FGateways;
	IRegistration *FRegistration;
private:
	QString FRegisterId;
	IGateServiceLabel FGateLabel;
	IGateServiceLogin FGateLogin;
private:
	Jid FStreamJid;
	Jid FServiceJid;
	Menu *FDomainsMenu;
	QString FAbortMessage;
};

#endif // ADDLEGACYACCOUNTDIALOG_H
