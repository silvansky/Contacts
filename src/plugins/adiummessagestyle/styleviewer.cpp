#include "styleviewer.h"

#include <QShortcut>
#include <QWebFrame>

StyleViewer::StyleViewer(QWidget *AParent) : QWebView(AParent)
{
	setPage(new WebPage(this));
	setAcceptDrops(false);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	QShortcut *shortcut = new QShortcut(QKeySequence::Copy, this,NULL,NULL,Qt::WidgetShortcut);
	connect(shortcut, SIGNAL(activated()), SLOT(onShortcutActivated()));
	connect(page(), SIGNAL(loadFinished(bool)), SLOT(onPageLoaded()));
}

StyleViewer::~StyleViewer()
{

}

QSize StyleViewer::sizeHint() const
{
	return QSize(256,192);
}

void StyleViewer::onShortcutActivated()
{
	triggerPageAction(QWebPage::Copy);
}

void StyleViewer::onPageLoaded()
{
	emit htmlChanged(this, page()->mainFrame()->toHtml());
}
