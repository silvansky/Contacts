#include "smsinfowidget.h"

SmsInfoWidget::SmsInfoWidget(ISmsMessageHandler *ASmsHandler, IChatWindow *AWindow, QWidget *AParent) : QFrame(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SMSMESSAGEHANDLER_INFOWIDGET);

	FChatWindow = AWindow;
	FSmsHandler = ASmsHandler;

	ui.lblRefill->setText(QString("<a href='%1'>%2</a>").arg("http://id.rambler.ru").arg(tr("Refill")));

	connect(FChatWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onEditWidgetTextChanged()));
	connect(FSmsHandler->instance(),SIGNAL(smsBalanceChanged(const Jid &, const Jid &, int)),SLOT(onSmsBalanceChanged(const Jid &, const Jid &, int)));

	onEditWidgetTextChanged();
	onSmsBalanceChanged(FChatWindow->streamJid(),FChatWindow->contactJid().domain(),FSmsHandler->smsBalance(FChatWindow->streamJid(),FChatWindow->contactJid().domain()));
}

SmsInfoWidget::~SmsInfoWidget()
{

}

IChatWindow *SmsInfoWidget::chatWindow() const
{
	return FChatWindow;
}

void SmsInfoWidget::onEditWidgetTextChanged()
{
	QTextEdit *editor = FChatWindow->editWidget()->textEdit();
	
	QString sms = editor->toPlainText();
	int chars = sms.length();
	int maxChars = sms.toUtf8()==sms.toLatin1() ? 120 : 60;
	ui.lblCharacters->setText(tr("<b>%1</b> from %2 characters").arg(chars).arg(maxChars));
	ui.lblCharacters->setVisible(!sms.isEmpty());
	FChatWindow->editWidget()->setSendButtonEnabled(chars>0 && chars<=maxChars);
}

void SmsInfoWidget::onSmsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance)
{
	if (AStreamJid==FChatWindow->streamJid() && AServiceJid==FChatWindow->contactJid().domain())
	{
		if (ABalance > 0)
		{
			ui.lblRefill->setVisible(true);
			ui.lblBalance->setText(tr("Balance: <b>%1 SMS</b>").arg(ABalance));
		}
		else if (ABalance == 0)
		{
			ui.lblRefill->setVisible(true);
			ui.lblBalance->setText(tr("You have run out of SMS"));
		}
		else
		{
			ui.lblRefill->setVisible(false);
			ui.lblBalance->setText(tr("SMS service is unavailable"));
		}
	}
}
