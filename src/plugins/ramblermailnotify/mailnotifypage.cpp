#include "mailnotifypage.h"

#include <QDesktopServices>

enum MailColumns {
	CMN_ICON,
	CMN_FROM,
	CMN_SUBJECT,
	CMN_DATE,
	CMN_COUNT
};

MailNotifyPage::MailNotifyPage(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERMAILNOTIFY_MAILNOTIFYPAGE);

	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	ui.twtMails->setColumnCount(CMN_COUNT);
	ui.twtMails->verticalHeader()->hide();
	ui.twtMails->horizontalHeader()->setHighlightSections(false);
	ui.twtMails->setHorizontalHeaderLabels(QStringList() << QString::null << tr("From") << tr("Subject") << tr("Date"));

	connect(ui.pbtNewMail,SIGNAL(clicked()),SLOT(onNewMailButtonClicked()));
	connect(ui.pbtIncoming,SIGNAL(clicked()),SLOT(onNewMailButtonClicked()));
}

MailNotifyPage::~MailNotifyPage()
{
	emit tabPageDestroyed();
}

void MailNotifyPage::showTabPage()
{
	if (FMessageWidgets && isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MailNotifyPage::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

bool MailNotifyPage::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

QString MailNotifyPage::tabPageId() const
{
	return "MailNotifyPage|"+FStreamJid.pBare()+"|"+FServiceJid.pBare();

}

QIcon MailNotifyPage::tabPageIcon() const
{
	return windowIcon();
}

QString MailNotifyPage::tabPageCaption() const
{
	return windowIconText();
}

QString MailNotifyPage::tabPageToolTip() const
{
	return FTabPageToolTip;
}

ITabPageNotifier *MailNotifyPage::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void MailNotifyPage::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

Jid MailNotifyPage::streamJid() const
{
	return FStreamJid;
}

Jid MailNotifyPage::serviceJid() const
{
	return FServiceJid;
}

bool MailNotifyPage::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QWidget::event(AEvent);
}

void MailNotifyPage::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);
	emit tabPageActivated();
}

void MailNotifyPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}

void MailNotifyPage::onNewMailButtonClicked()
{
	QDesktopServices::openUrl(QString("http://mail.rambler.ru/mail/compose.cgi"));
}

void MailNotifyPage::onIncomingButtonClicked()
{
	QDesktopServices::openUrl(QString("http://mail.rambler.ru/mail/mailbox.cgi?mbox=INBOX"));
}
