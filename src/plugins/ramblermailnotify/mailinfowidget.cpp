#include "mailinfowidget.h"

#include <QDesktopServices>

MailInfoWidget::MailInfoWidget(IChatWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SMSMESSAGEHANDLER_INFOWIDGET);

	QString dest = AWindow->contactJid().uNode();
	if (dest.lastIndexOf('%')>=0)
		dest[dest.lastIndexOf('%')] = '@';
	ui.lblInfo->setText(tr("From:%1    To:%2").arg(AWindow->streamJid().uBare()).arg(dest));

	ui.lblIncoming->setText(QString("<a href='http://mail.rambler.ru/mail/mailbox.cgi?mbox=INBOX'>%1</a>").arg(tr("Incoming")));
}

MailInfoWidget::~MailInfoWidget()
{

}
