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
protected slots:
	void onEditWidgetTextChanged();
	void onSmsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance);
private:
	Ui::SmsInfoWidget ui;
private:
	IChatWindow *FChatWindow;
	ISmsMessageHandler *FSmsHandler;
private:
	QKeySequence FSendKey;
};

#endif // SMSINFOWIDGET_H
