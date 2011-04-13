#include "addmetaitempage.h"

AddMetaItemPage::AddMetaItemPage(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
}

AddMetaItemPage::~AddMetaItemPage()
{
	emit tabPageDestroyed();
}

void AddMetaItemPage::showTabPage()
{
	emit tabPageShow();
}

void AddMetaItemPage::closeTabPage()
{
	emit tabPageClose();
}

bool AddMetaItemPage::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

QString AddMetaItemPage::tabPageId() const
{
	return "AddMetaTabPage|"/*+FMetaRoster->streamJid().pBare()+"|"+FMetaId*/;
}

QIcon AddMetaItemPage::tabPageIcon() const
{
	return windowIcon();
}

QString AddMetaItemPage::tabPageCaption() const
{
	return windowIconText();
}

QString AddMetaItemPage::tabPageToolTip() const
{
	return QString::null;
}

ITabPageNotifier *AddMetaItemPage::tabPageNotifier() const
{
	return NULL;
}

void AddMetaItemPage::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	Q_UNUSED(ANotifier);
}

bool AddMetaItemPage::event(QEvent *AEvent)
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

void AddMetaItemPage::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);
	emit tabPageActivated();
}

void AddMetaItemPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}
