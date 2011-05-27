#include "smsinfowidget.h"

SmsInfoWidget::SmsInfoWidget(ISmsMessageHandler *ASmsHandler, IChatWindow *AWindow, QWidget *AParent) : QFrame(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SMSMESSAGEHANDLER_INFOWIDGET);

	FChatWindow = AWindow;
	FSmsHandler = ASmsHandler;

	FSendKey = FChatWindow->editWidget()->sendKey();

	ui.lblPhoneNumber->setText(AWindow->contactJid().node());
	ui.lblRefill->setText(QString("<a href='%1'>%2</a>").arg("http://id.rambler.ru").arg(tr("Refill")));

	connect(FChatWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onEditWidgetTextChanged()));
	connect(FSmsHandler->instance(),SIGNAL(smsBalanceChanged(const Jid &, const Jid &, int)),SLOT(onSmsBalanceChanged(const Jid &, const Jid &, int)));

	FBalance = FSmsHandler->smsBalance(FChatWindow->streamJid(),FChatWindow->contactJid().domain());
	if (FBalance < 0)
		FSmsHandler->requestSmsBalance(FChatWindow->streamJid(),FChatWindow->contactJid().domain());

	onEditWidgetTextChanged();
	onSmsBalanceChanged(FChatWindow->streamJid(),FChatWindow->contactJid().domain(),FBalance);
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
	QString smsText = editor->toPlainText();
	int chars = smsText.length();
	int maxChars = smsText.toUtf8()==smsText.toLatin1() ? 120 : 60;
	ui.lblCharacters->setVisible(chars>0);
	ui.lblCharacters->setText(tr("<b>%1</b> from %2 characters").arg(chars).arg(maxChars));

	bool isError = chars>maxChars;
	if (isError != ui.lblCharacters->property("error").toBool())
	{
		ui.lblCharacters->setProperty("error", isError ? true : false);
		StyleStorage::updateStyle(this);
	}

	FChatWindow->editWidget()->setSendButtonEnabled(FBalance>0 && chars>0 && chars<=maxChars);
	FChatWindow->editWidget()->setSendKey(chars>0 && chars<=maxChars ? FSendKey : QKeySequence::UnknownKey);
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

		FBalance = ABalance;
		FChatWindow->editWidget()->setSendButtonEnabled(FBalance>0);
		FChatWindow->editWidget()->textEdit()->setEnabled(FBalance>0);

		ui.lblBalance->setProperty("error", FBalance>0 ? false : true);
		StyleStorage::updateStyle(this);
	}
}
