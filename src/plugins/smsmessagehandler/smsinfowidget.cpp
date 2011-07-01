#include "smsinfowidget.h"

SmsInfoWidget::SmsInfoWidget(ISmsMessageHandler *ASmsHandler, IChatWindow *AWindow, QWidget *AParent) : QFrame(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SMSMESSAGEHANDLER_INFOWIDGET);

	FChatWindow = AWindow;
	FSmsHandler = ASmsHandler;

	FSupplementMode = false;
	FSendKey = FChatWindow->editWidget()->sendKey();
	FErrorMessage = Qt::escape(tr("SMS service is unavailable, please try later."));

	ui.lblPhoneNumber->setText(AWindow->contactJid().node());
	ui.lblSupplement->setText(QString("<a href='%1'>%2</a>").arg("http://id.rambler.ru").arg(tr("Supplement")));
	connect(ui.lblSupplement,SIGNAL(linkActivated(const QString &)),SLOT(onSupplementLinkActivated()));
	connect(FChatWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onEditWidgetTextChanged()));
	connect(FChatWindow->editWidget()->textEdit(),SIGNAL(anchorClicked(const QUrl &)),SLOT(onSupplementLinkActivated()));

	connect(FSmsHandler->instance(),SIGNAL(smsSupplementReceived(const QString &, const QString &, const QString &, int)),
		SLOT(onSmsSupplementReceived(const QString &, const QString &, const QString &, int)));
	connect(FSmsHandler->instance(),SIGNAL(smsSupplementError(const QString &, const QString &, const QString &)),
		SLOT(onSmsSupplementError(const QString &, const QString &, const QString &)));
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

void SmsInfoWidget::showStyledStatus(const QString &AHtml)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Status;
	options.time = QDateTime::currentDateTime();
	options.timeFormat = " ";
	options.direction = IMessageContentOptions::DirectionIn;
	options.senderId = FChatWindow->contactJid().pDomain();
	options.senderName = tr("SMS Service");
	FChatWindow->viewWidget()->changeContentHtml(AHtml,options);
}

void SmsInfoWidget::onEditWidgetTextChanged()
{
	QTextEdit *editor = FChatWindow->editWidget()->textEdit();
	QString smsText = editor->toPlainText();
	int chars = smsText.length();
	int maxChars = smsText.toUtf8()==smsText.toLatin1() ? 120 : 60;
	ui.lblCharacters->setVisible(!FSupplementMode && chars>0);
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

void SmsInfoWidget::onSupplementLinkActivated()
{
	if (FSupplementRequest.isEmpty())
	{
		FSupplementRequest = FSmsHandler->requestSmsSupplement(FChatWindow->streamJid(),FChatWindow->contactJid().domain());
		if (FSupplementRequest.isEmpty())
		{
			if (FSupplementMode)
			{
				FSupplementMessage = FErrorMessage;
				onSmsBalanceChanged(FChatWindow->streamJid(),FChatWindow->contactJid().domain(),FBalance);
			}
			else
			{
				showStyledStatus(FErrorMessage);
			}
		}
	}
}

void SmsInfoWidget::onSmsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance)
{
	if (AStreamJid==FChatWindow->streamJid() && AServiceJid==FChatWindow->contactJid().domain())
	{
		if (ABalance >= 0)
		{
			ui.lblSupplement->setVisible(true);
			ui.lblBalance->setText(tr("Balance: <b>%1 SMS</b>").arg(ABalance));
		}
		else
		{
			ui.lblSupplement->setVisible(false);
			ui.lblBalance->setText(tr("SMS service is unavailable"));
		}

		if (ABalance == 0)
		{
			static const QString html = "<div align='center'><br>%1<br>%2</div>";

			if (!FSupplementMode)
			{
				FSupplementMode = true;
				ui.lblCharacters->setVisible(false);

				FFormatEnabled = FChatWindow->editWidget()->textFormatEnabled();
				FChatWindow->editWidget()->setTextFormatEnabled(true);

				FInteractionFlags = FChatWindow->editWidget()->textEdit()->textInteractionFlags();
				FChatWindow->editWidget()->textEdit()->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
			}

			FChatWindow->editWidget()->textEdit()->setHtml(html
				.arg(Qt::escape(tr("You have run out of SMS. Please, %1."))
				.arg(QString("<a href='supplement'><span style='color:#acacac;'>%1</span></a>")
				.arg(Qt::escape(tr("supplement the balance")))))
				.arg(FSupplementMessage));

		}
		else if (ABalance!=0 && FSupplementMode)
		{
			FSupplementMode = false;
			FSupplementMessage.clear();
			ui.lblCharacters->setVisible(true);
			FChatWindow->editWidget()->clearEditor();
			FChatWindow->editWidget()->setTextFormatEnabled(FFormatEnabled);
			FChatWindow->editWidget()->textEdit()->setTextInteractionFlags(FInteractionFlags);
		}

		FBalance = ABalance;

		FChatWindow->editWidget()->setSendButtonEnabled(FBalance>0);
		FChatWindow->editWidget()->textEdit()->setReadOnly(FBalance<=0);

		ui.lblBalance->setProperty("error", FBalance>0 ? false : true);
		StyleStorage::updateStyle(this);
	}
}

void SmsInfoWidget::onSmsSupplementReceived(const QString &AId, const QString &ANumber, const QString &ACode, int ACount)
{
	if (FSupplementRequest == AId)
	{
		QString message = Qt::escape(tr("Send a SMS message with the code %2 to phone number %1 to supplement your balance at %3 SMS.").arg(ANumber).arg(ACode).arg(ACount));
		if (FSupplementMode)
		{
			FSupplementMessage = message;
			onSmsBalanceChanged(FChatWindow->streamJid(),FChatWindow->contactJid().domain(),FBalance);
		}
		else
		{
			showStyledStatus(message);
		}
		FSupplementRequest.clear();
	}
}

void SmsInfoWidget::onSmsSupplementError(const QString &AId, const QString &ACondition, const QString &AMessage)
{
	Q_UNUSED(ACondition); Q_UNUSED(AMessage);
	if (FSupplementRequest == AId)
	{
		if (FSupplementMode)
		{
			FSupplementMessage = FErrorMessage;
			onSmsBalanceChanged(FChatWindow->streamJid(),FChatWindow->contactJid().domain(),FBalance);
		}
		else
		{
			showStyledStatus(FErrorMessage);
		}
		FSupplementRequest.clear();
	}
}
