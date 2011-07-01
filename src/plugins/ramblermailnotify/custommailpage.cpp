#include "custommailpage.h"

#include <QPainter>
#include <QPaintEvent>

CustomMailPage::CustomMailPage(IGateways *AGateways, IMessageWidgets *AMessageWidgets, IRosterIndex *AMailIndex, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Write a mail"));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERMAILNOTIFY_CUSTOMMAILPAGE);

	FGateways = AGateways;
	FMessageWidgets = AMessageWidgets;

	FMailIndex = AMailIndex;
	FServiceJid = AServiceJid;
	FDescriptor = FGateways->serviceDescriptor(streamJid(),serviceJid());

	ui.pbtContinue->setEnabled(false);
	ui.pbtContinue->addTextFlag(TF_LIGHTSHADOW);
	connect(ui.pbtContinue,SIGNAL(clicked()),SLOT(onContinueButtonClicked()));
	connect(ui.lneContactMail,SIGNAL(textChanged(const QString &)),SLOT(onContactMailChanged(const QString &)));

	connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),
		SLOT(onUserJidReceived(const QString &, const Jid &)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),
		SLOT(onErrorReceived(const QString &, const QString &)));

	connect(FMailIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),SLOT(onMailIndexDataChanged(IRosterIndex *, int)));

	onMailIndexDataChanged(FMailIndex,Qt::DecorationRole);
}

CustomMailPage::~CustomMailPage()
{
	emit tabPageDestroyed();
}

void CustomMailPage::showTabPage()
{
	if (FMessageWidgets && isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void CustomMailPage::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

bool CustomMailPage::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

QString CustomMailPage::tabPageId() const
{
	return "CustomMailPage|"+streamJid().pBare()+"|"+serviceJid().pBare();
}

QIcon CustomMailPage::tabPageIcon() const
{
	return windowIcon();
}

QString CustomMailPage::tabPageCaption() const
{
	return windowTitle();
}

QString CustomMailPage::tabPageToolTip() const
{
	return QString::null;
}

ITabPageNotifier *CustomMailPage::tabPageNotifier() const
{
	return NULL;
}

void CustomMailPage::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	Q_UNUSED(ANotifier);
}

Jid CustomMailPage::streamJid() const
{
	return FMailIndex->data(RDR_STREAM_JID).toString();
}

Jid CustomMailPage::serviceJid() const
{
	return FServiceJid;
}

bool CustomMailPage::event(QEvent *AEvent)
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

void CustomMailPage::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);
	emit tabPageActivated();
}

void CustomMailPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}

void CustomMailPage::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CustomMailPage::onContinueButtonClicked()
{
	QString contact = FGateways->normalizedContactLogin(FDescriptor,ui.lneContactMail->text().trimmed());
	FRequestId = FGateways->sendUserJidRequest(streamJid(),serviceJid(),contact);
	if (!FRequestId.isEmpty())
	{
		ui.pbtContinue->setEnabled(false);
		ui.lneContactMail->setEnabled(false);
	}
}

void CustomMailPage::onContactMailChanged(const QString &AText)
{
	QString contact = FGateways->normalizedContactLogin(FDescriptor,AText.trimmed());
	QString error = FGateways->checkNormalizedContactLogin(FDescriptor,contact);
	ui.pbtContinue->setEnabled(error.isNull());
}

void CustomMailPage::onMailIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex==FMailIndex && ARole==Qt::DecorationRole)
	{
		setWindowIcon(FMailIndex->data(Qt::DecorationRole).value<QIcon>());
		emit tabPageChanged();
	}
}

void CustomMailPage::onUserJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (AId == FRequestId)
	{
		emit showChatWindow(AUserJid);
		close();
	}
}

void CustomMailPage::onErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (AId == FRequestId)
	{
		ui.pbtContinue->setEnabled(true);
		ui.lneContactMail->setEnabled(true);
	}
}
