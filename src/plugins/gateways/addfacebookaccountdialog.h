#ifndef ADDFACEBOOKACCOUNTDIALOG_H
#define ADDFACEBOOKACCOUNTDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <interfaces/ipresence.h>
#include <interfaces/igateways.h>
#include <interfaces/iregistraton.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <utils/log.h>
#include <utils/stylestorage.h>
#include <utils/custominputdialog.h>
#include <utils/customborderstorage.h>
#include "ui_addfacebookaccountdialog.h"

class AddFacebookAccountDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	AddFacebookAccountDialog(IGateways *AGateways, IRegistration *ARegistration, IPresence *APresence, const Jid &AServiceJid, QWidget *AParent = NULL);
	~AddFacebookAccountDialog();
protected:
	void checkResult();
	void abort(const QString &AMessage);
	void setWaitMode(bool AWait, const QString &AMessage = QString::null);
protected slots:
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterSuccess(const QString &AId);
	void onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage);
	void onWebViewLoadStarted();
	void onWebViewLoadFinished(bool AOk);
	void onWebPageLinkClicked(const QUrl &AUrl);
private:
	Ui::AddFacebookAccountDialogClass ui;
private:
	IPresence *FPresence;
	IGateways *FGateways;
	IRegistration *FRegistration;
private:
	Jid FServiceJid;
	QString FRegisterId;
	QString FAbortMessage;
	IGateServiceLogin FGateLogin;
};

#endif // ADDFACEBOOKACCOUNTDIALOG_H
