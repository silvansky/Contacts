#include "mailnotifypage.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDesktopServices>

#define TDR_CONTACT_JID     Qt::UserRole+1

enum MailColumns {
	CMN_FROM,
	CMN_SUBJECT,
	CMN_DATE,
	CMN_COUNT
};

MailNotifyPage::MailNotifyPage(IMessageWidgets *AMessageWidgets, IRosterIndex *AMailIndex, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setWindowTitle(tr("New e-mails"));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERMAILNOTIFY_MAILNOTIFYPAGE);

	FMessageWidgets = AMessageWidgets;

	FMailIndex = AMailIndex;
	FServiceJid = AServiceJid;
	FTabPageNotifier = NULL;

	ui.twtMails->setColumnCount(CMN_COUNT);
	ui.twtMails->horizontalHeader()->setVisible(false);
	ui.twtMails->horizontalHeader()->setHighlightSections(false);
	ui.twtMails->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	ui.twtMails->setHorizontalHeaderLabels(QStringList() << tr("From") << tr("Subject") << tr("Time"));
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_FROM,QHeaderView::ResizeToContents);
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_SUBJECT,QHeaderView::Stretch);
	ui.twtMails->horizontalHeader()->setResizeMode(CMN_DATE,QHeaderView::ResizeToContents);
	connect(ui.twtMails,SIGNAL(cellDoubleClicked(int,int)),SLOT(onTableCellDoubleClicked(int,int)));

	ui.pbtNewMail->addTextFlag(TF_LIGHTSHADOW);
	connect(ui.pbtNewMail,SIGNAL(clicked()),SLOT(onNewMailButtonClicked()));

	ui.pbtGoToEmail->addTextFlag(TF_LIGHTSHADOW);
	connect(ui.pbtGoToEmail,SIGNAL(clicked()),SLOT(onGoToEmailButtonClicked()));

	connect(FMailIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),SLOT(onMailIndexDataChanged(IRosterIndex *, int)));

	clearNewMails();
	onMailIndexDataChanged(FMailIndex,Qt::DecorationRole);
}

MailNotifyPage::~MailNotifyPage()
{
	clearNewMails();
	emit tabPageDestroyed();
}

void MailNotifyPage::assignTabPage()
{
	if (FMessageWidgets && isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);
	else
		emit tabPageAssign();
}

void MailNotifyPage::showTabPage()
{
	assignTabPage();
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MailNotifyPage::showMinimizedTabPage()
{
	assignTabPage();
	if (isWindow() && !isVisible())
		showMinimized();
	else
		emit tabPageShowMinimized();
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
	return QString::null;
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

int MailNotifyPage::newMailsCount() const
{
	return ui.twtMails->rowCount();
}

void MailNotifyPage::appendNewMail(const Stanza &AStanza)
{
	Message message(AStanza);
	QDomElement contactElem = AStanza.firstElement("x",NS_RAMBLER_MAIL_NOTICE).firstChildElement("contact");

	QTableWidgetItem *fromItem = new QTableWidgetItem();
	QString fromName = contactElem.firstChildElement("name").text().trimmed();
	//fromItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RAMBLERMAILNOTIFY_MAIL));
	fromItem->setText(fromName.isEmpty() ? contactElem.firstChildElement("e-mail").text() : fromName);
	fromItem->setData(Qt::UserRole,contactElem.firstChildElement("jid").text());
	
	QTableWidgetItem *subjectItem = new QTableWidgetItem();
	subjectItem->setText(message.subject());

	QTableWidgetItem *dateItem = new QTableWidgetItem();
	dateItem->setText(message.dateTime().time().toString("hh:mm"));

	QFont font = fromItem->font();
	font.setBold(true);
	fromItem->setFont(font);
	subjectItem->setFont(font);
	dateItem->setFont(font);

	ui.twtMails->setRowCount(ui.twtMails->rowCount()+1);
	ui.twtMails->setItem(ui.twtMails->rowCount()-1,CMN_FROM,fromItem);
	ui.twtMails->setItem(fromItem->row(),CMN_SUBJECT,subjectItem);
	ui.twtMails->setItem(fromItem->row(),CMN_DATE,dateItem);

	ui.lblNoMail->setVisible(false);
	ui.twtMails->setVisible(true);
}

void MailNotifyPage::clearNewMails()
{
	ui.twtMails->clearContents();
	ui.twtMails->setRowCount(0);

	ui.lblNoMail->setVisible(true);
	ui.twtMails->setVisible(false);
}

void MailNotifyPage::clearBoldFont()
{
	for (int row=0; row<ui.twtMails->rowCount(); row++)
	{
		for(int cmn=0; cmn<CMN_COUNT; cmn++)
		{
			QTableWidgetItem *item = ui.twtMails->item(row,cmn);
			if (item)
			{
				QFont font = item->font();
				font.setBold(false);
				item->setFont(font);

				if (cmn != CMN_FROM)
					item->setForeground(Qt::gray);
			}
		}
	}
}

bool MailNotifyPage::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		clearBoldFont();
		emit tabPageDeactivated();
	}
	return QWidget::event(AEvent);
}

void MailNotifyPage::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);
	if (isActive())
		emit tabPageActivated();
}

void MailNotifyPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}

void MailNotifyPage::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void MailNotifyPage::onNewMailButtonClicked()
{
	emit showCustomMailPage();
}

void MailNotifyPage::onGoToEmailButtonClicked()
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

void MailNotifyPage::onMailIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex==FMailIndex && ARole==Qt::DecorationRole)
	{
		setWindowIcon(FMailIndex->data(Qt::DecorationRole).value<QIcon>());
		emit tabPageChanged();
	}
}
