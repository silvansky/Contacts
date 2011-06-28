#ifndef SMSINFOWIDGET_H
#define SMSINFOWIDGET_H

#include <QFrame>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/ismsmessagehandler.h>
#include <interfaces/imessagewidgets.h>
#include <utils/stylestorage.h>
#include "ui_smsinfowidget.h"

class SmsInfoWidget : 
	public QFrame
{
	Q_OBJECT;
public:
	SmsInfoWidget(ISmsMessageHandler *ASmsHandler, IChatWindow *AWindow, QWidget *AParent = NULL);
	~SmsInfoWidget();
	IChatWindow *chatWindow() const;
protected:
	void showStyledStatus(const QString &AHtml);
protected slots:
	void onEditWidgetTextChanged();
	void onSupplementLinkActivated(const QString &ALink);
	void onSmsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance);
	void onSmsSupplementReceived(const QString &AId, const QString &ANumber, const QString &ACode, int ACount);
	void onSmsSupplementError(const QString &AId, const QString &ACondition, const QString &AMessage);
private:
	Ui::SmsInfoWidgetClass ui;
private:
	IChatWindow *FChatWindow;
	ISmsMessageHandler *FSmsHandler;
private:
	int FBalance;
	QString FErrorMessage;
	QKeySequence FSendKey;
	QString FSupplementRequest;
};

#endif // SMSINFOWIDGET_H
