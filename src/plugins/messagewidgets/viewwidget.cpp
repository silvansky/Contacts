#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QHBoxLayout>

ViewWidget::ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setAcceptDrops(true);

	QHBoxLayout *layout = new QHBoxLayout(ui.wdtViewer);
	layout->setMargin(0);
	layout->setSpacing(0);

	FMessageStyle = NULL;
	FMessageProcessor = NULL;
	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FStyleWidget = NULL;

	initialize();
}

ViewWidget::~ViewWidget()
{

}

void ViewWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid befour = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(befour);
	}
}

void ViewWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid befour = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(befour);
	}
}

QWidget *ViewWidget::styleWidget() const
{
	return FStyleWidget;
}

IMessageStyle *ViewWidget::messageStyle() const
{
	return FMessageStyle;
}

void ViewWidget::setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions)
{
	if (FMessageStyle != AStyle)
	{
		IMessageStyle *befour = FMessageStyle;
		FMessageStyle = AStyle;
		if (befour)
		{
			disconnect(befour->instance(),SIGNAL(contentChanged(QWidget *, const QUuid &, const QString &, const IMessageContentOptions &)),
				this, SLOT(onContentChanged(QWidget *, const QUuid &, const QString &, const IMessageContentOptions &)));
			disconnect(befour->instance(),SIGNAL(urlClicked(QWidget *, const QUrl &)),this,SLOT(onUrlClicked(QWidget *, const QUrl &)));
			disconnect(FStyleWidget,SIGNAL(customContextMenuRequested(const QPoint &)),this,SLOT(onCustomContextMenuRequested(const QPoint &)));
			ui.wdtViewer->layout()->removeWidget(FStyleWidget);
			FStyleWidget->deleteLater();
			FStyleWidget = NULL;
		}
		if (FMessageStyle)
		{

			FStyleWidget = FMessageStyle->createWidget(AOptions,ui.wdtViewer);
			FStyleWidget->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(FStyleWidget,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onCustomContextMenuRequested(const QPoint &)));
			connect(FMessageStyle->instance(),SIGNAL(contentChanged(QWidget *, const QUuid &, const QString &, const IMessageContentOptions &)),
				SLOT(onContentChanged(QWidget *, const QUuid &, const QString &, const IMessageContentOptions &)));
			connect(FMessageStyle->instance(),SIGNAL(urlClicked(QWidget *, const QUrl &)),SLOT(onUrlClicked(QWidget *, const QUrl &)));
			ui.wdtViewer->layout()->addWidget(FStyleWidget);
		}
		emit messageStyleChanged(befour,AOptions);
	}
}

QUuid ViewWidget::changeContentHtml(const QString &AHtml, const IMessageContentOptions &AOptions)
{
	return FMessageStyle ? FMessageStyle->changeContent(FStyleWidget,AHtml,AOptions) : QUuid();
}

QUuid ViewWidget::changeContentText(const QString &AText, const IMessageContentOptions &AOptions)
{
	Message message;
	message.setBody(AText);
	return changeContentMessage(message,AOptions);
}

QUuid ViewWidget::changeContentMessage(const Message &AMessage, const IMessageContentOptions &AOptions)
{
	QTextDocument messageDoc;
	if (FMessageProcessor)
		FMessageProcessor->messageToText(&messageDoc,AMessage);
	else
		messageDoc.setPlainText(AMessage.body());
	return changeContentHtml(getHtmlBody(messageDoc.toHtml()),AOptions);
}

void ViewWidget::contextMenuForView(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu)
{
	emit viewContextMenu(APosition,ASelection,AMenu);
}

void ViewWidget::initialize()
{
	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
}

QString ViewWidget::getHtmlBody(const QString &AHtml)
{
	QRegExp body("<body.*>(.*)</body>");
	body.setMinimal(false);
	return AHtml.indexOf(body)>=0 ? body.cap(1).trimmed() : AHtml;
}

void ViewWidget::dropEvent(QDropEvent *AEvent)
{
	Menu *dropMenu = new Menu(this);

	bool accepted = false;
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDropAction(this, AEvent, dropMenu))
			accepted = true;

	QList<Action *> actionList = dropMenu->groupActions();
	if (accepted && !actionList.isEmpty())
	{
		QAction *action = !(AEvent->mouseButtons() & Qt::RightButton) && actionList.count()==1 ? actionList.value(0) : NULL;
		if (action)
			action->trigger();
		else
			action = dropMenu->exec(mapToGlobal(AEvent->pos()));

		if (action)
			AEvent->acceptProposedAction();
		else
			AEvent->ignore();
	}
	else
		AEvent->ignore();

	delete dropMenu;
}

void ViewWidget::dragEnterEvent(QDragEnterEvent *AEvent)
{
	FActiveDropHandlers.clear();
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDragEnter(this, AEvent))
			FActiveDropHandlers.append(handler);

	if (!FActiveDropHandlers.isEmpty())
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void ViewWidget::dragMoveEvent(QDragMoveEvent *AEvent)
{
	bool accepted = false;
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDragMove(this, AEvent))
			accepted = true;

	if (accepted)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void ViewWidget::dragLeaveEvent(QDragLeaveEvent *AEvent)
{
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		handler->viewDragLeave(this, AEvent);
}

void ViewWidget::onContentChanged(QWidget *AWidget, const QUuid &AContentId, const QString &AHtml, const IMessageContentOptions &AOptions)
{
	if (AWidget == FStyleWidget)
		emit contentChanged(AContentId,AHtml,AOptions);
}

void ViewWidget::onUrlClicked(QWidget *AWidget, const QUrl &AUrl)
{
	if (AWidget == FStyleWidget)
		emit urlClicked(AUrl);
}

void ViewWidget::onCustomContextMenuRequested(const QPoint &APosition)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose, true);

	contextMenuForView(APosition,FMessageStyle->selection(FStyleWidget),menu);

	if (!menu->isEmpty())
		menu->popup(FStyleWidget->mapToGlobal(APosition));
	else
		delete menu;
}
