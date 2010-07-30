#include "masssenddialog.h"
#include "ui_masssenddialog.h"
#include <utils/widgetmanager.h>

MassSendDialog::MassSendDialog(IMessageWidgets *AMessageWidgets, const Jid & AStreamJid, QWidget *parent) :
		QDialog(parent),
		ui(new Ui::MassSendDialog),
		FStreamJid(AStreamJid)
{
	ui->setupUi(this);
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
	{
		isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
		WidgetManager::raiseWidget(this);
	}
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
