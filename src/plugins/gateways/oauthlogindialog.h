#ifndef OAUTHLOGINDIALOG_H
#define OAUTHLOGINDIALOG_H

#include <QUrl>
#include <QDialog>
#include <interfaces/ipresence.h>
#include <interfaces/igateways.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include "ui_oauthlogindialog.h"

class OAuthLoginDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	OAuthLoginDialog(IPresence *APresence, const QUrl &AAuthUrl, const IGateServiceLabel &AGateLabel, const Jid &AServiceJid, QWidget *AParent = NULL);
	~OAuthLoginDialog();
	QString errorString() const;
	QMap<QString, QString> urlItems() const;
protected:
	void checkResult();
	void abort(const QString &AMessage);
	void setWaitMode(bool AWait, const QString &AMessage = QString::null);
protected slots:
	void onWebViewLoadStarted();
	void onWebViewLoadFinished(bool AOk);
	void onWebPageLinkClicked(const QUrl &AUrl);
private:
	Ui::OAuthLoginDialogClass ui;
private:
	IGateServiceLabel FGateLabel;
private:
	Jid FServiceJid;
	QString FErrorString;
	QMap<QString, QString> FUrlItems;
};

#endif // OAUTHLOGINDIALOG_H
