#include "metatabwidget.h"

MetaTabWidget::MetaTabWidget(IMessageWidgets *AMessageWidgets, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;
	FMessageWidgets = AMessageWidgets;

	FTabPageNotifier = NULL;
}

MetaTabWidget::~MetaTabWidget()
{
	emit tabPageDestroyed();
}

void MetaTabWidget::showTabPage()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MetaTabWidget::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QString MetaTabWidget::tabPageId() const
{
	return "MetaTabWidget|"+FMetaRoster->streamJid().pBare()+"|"+FMetaId.pBare();
}

bool MetaTabWidget::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

ITabPageNotifier *MetaTabWidget::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void  MetaTabWidget::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}
