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
	ui->messagingLayout->addWidget(FViewWidget->instance());
	ui->messagingLayout->addWidget(FEditWidget->instance());
	ui->recieversLayout->addWidget(FReceiversWidget->instance());
}

MassSendDialog::~MassSendDialog()
{
	delete ui;
}

const Jid &MassSendDialog::streamJid() const
{
	return FStreamJid;
}

IViewWidget *MassSendDialog::viewWidget() const
{
	return FViewWidget;
}

IEditWidget *MassSendDialog::editWidget() const
{
	return FEditWidget;
}

IReceiversWidget *MassSendDialog::receiversWidget() const
{
	return FReceiversWidget;
}

void MassSendDialog::onMessageReady()
{
	emit messageReady();
}
