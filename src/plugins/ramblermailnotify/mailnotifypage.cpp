#include "mailnotifypage.h"

#include <QDesktopServices>

#define TDR_CONTACT_JID     Qt::UserRole+1

enum MailColumns {
	CMN_ICON,
	CMN_FROM,
	CMN_SUBJECT,
	CMN_DATE,
	CMN_COUNT
};

MailNotifyPage::MailNotifyPage(IMessageWidgets *AMessageWidgets, IRosterIndex *AMailIndex, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowTitle(tr("New e-mail messages"));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERMAILNOTIFY_MAILNOTIFYPAGE);

	FMessageWidgets = AMessageWidgets;

	FMailIndex = AMailIndex;
	FServiceJid = AServiceJid;
	FTabPageNotifier = NULL;

	ui.twtMails->setColumnCount(CMN_COUNT);
	ui.twtMails->verticalHeader()->hide();
	ui.twtMails->horizontalHeader()->setHighlightSections(false);
	ui.twtMails->setHorizontalHeaderLabels(QStringList() << QString::null << tr("From") << tr("Subject") << tr("Time"));

	ui.twtMails->horizontalHeader()->setResizeMode(CMN_ICON,QHeaderView::ResizeToContents);
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_FROM,QHeaderView::ResizeToContents);
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_SUBJECT,QHeaderView::Stretch);
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_DATE,QHeaderView::ResizeToContents);
	connect(ui.twtMails,SIGNAL(cellDoubleClicked(int,int)),SLOT(onTableCellDoubleClicked(int,int)));

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
	return "MailNotifyPage|"+streamJid().pBare()+"|"+serviceJid().pBare();
}

QIcon MailNotifyPage::tabPageIcon() const
{
	return FMailIndex->data(Qt::DecorationRole).value<QIcon>();
}

QString MailNotifyPage::tabPageCaption() const
{
	return tr("Mails");
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
	return FMailIndex->data(RDR_STREAM_JID).toString();
}

Jid MailNotifyPage::serviceJid() const
{
	return FServiceJid;
}

void MailNotifyPage::appendNewMail(const Stanza &AStanza)
{
	Message message(AStanza);
	QDomElement xElem = AStanza.firstElement("x",NS_RAMBLER_MAIL_NOTIFY);

	QTableWidgetItem *iconItem = new QTableWidgetItem();
	iconItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RAMBLERMAILNOTIFY_MAIL));

	QTableWidgetItem *fromItem = new QTableWidgetItem();
	fromItem->setText(xElem.firstChildElement("from").text());
	fromItem->setData(Qt::UserRole,xElem.firstChildElement("contact").text());

	QTableWidgetItem *subjectItem = new QTableWidgetItem();
	subjectItem->setText(message.subject());

	QTableWidgetItem *dateItem = new QTableWidgetItem();
	dateItem->setText(message.dateTime().time().toString());

	ui.twtMails->setRowCount(ui.twtMails->rowCount()+1);
	ui.twtMails->setItem(ui.twtMails->rowCount()-1,CMN_ICON,iconItem);
	ui.twtMails->setItem(iconItem->row(),CMN_FROM,fromItem);
	ui.twtMails->setItem(iconItem->row(),CMN_SUBJECT,subjectItem);
	ui.twtMails->setItem(iconItem->row(),CMN_DATE,dateItem);
}

void MailNotifyPage::clearNewMails()
{
	ui.twtMails->clearContents();
	ui.twtMails->setRowCount(0);
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

void MailNotifyPage::onTableCellDoubleClicked(int ARow, int AColumn)
{
	Q_UNUSED(AColumn);
	QTableWidgetItem *fromItem = ui.twtMails->item(ARow,CMN_FROM);
	if (fromItem)
		emit showChatWindow(fromItem->data(Qt::UserRole).toString());
}
