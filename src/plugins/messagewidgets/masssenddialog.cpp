#include "masssenddialog.h"
#include "ui_masssenddialog.h"
#include <utils/widgetmanager.h>

MassSendDialog::MassSendDialog(IMessageWidgets *AMessageWidgets, const Jid & AStreamJid, QWidget *parent) :
		QDialog(parent),
		ui(new Ui::MassSendDialog),
		FStreamJid(AStreamJid)
{
	ui->setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MESSAGEWIDGETS_MASSENDDIALOG);

	FMessageWidgets = AMessageWidgets;
	FViewWidget = FMessageWidgets->newViewWidget(AStreamJid, Jid());
	FEditWidget = FMessageWidgets->newEditWidget(AStreamJid, Jid());
	connect(FEditWidget->instance(), SIGNAL(messageReady()), SLOT(onMessageReady()));
	FEditWidget->setSendKey(QKeySequence(Qt::Key_Return));
	FReceiversWidget = FMessageWidgets->newReceiversWidget(AStreamJid);
	FTabPageNotifier = FMessageWidgets->newTabPageNotifier(this);
	ui->messagingLayout->addWidget(FViewWidget->instance());
	ui->messagingLayout->addWidget(FEditWidget->instance());
	ui->recieversLayout->addWidget(FReceiversWidget->instance());
}

MassSendDialog::~MassSendDialog()
{
	delete ui;
}

void MassSendDialog::showTabPage()
{
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MassSendDialog::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QString MassSendDialog::tabPageId() const
{
	return "MassSendDialog|"+FStreamJid.pBare();
}

bool MassSendDialog::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

void MassSendDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MassSendDialog::onMessageReady()
{
//	IMessageContentOptions options;
//	options.direction = IMessageContentOptions::DirectionOut;
//	FViewWidget->appendText(FEditWidget->textEdit()->toPlainText(), options);
	emit messageReady();
}
