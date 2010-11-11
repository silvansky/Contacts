#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MESSAGEWIDGETS_CHATWINDOW);

	FStatusChanger = NULL;
	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FShownDetached = false;

	FTabPageNotifier = NULL;

	FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
	ui.wdtInfo->setLayout(new QVBoxLayout);
	ui.wdtInfo->layout()->setMargin(0);
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
	onOptionsChanged(Options::node(OPV_MESSAGES_SHOWINFOWIDGET));

	FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid);
	ui.wdtView->setLayout(new QVBoxLayout);
	ui.wdtView->layout()->setMargin(0);
	ui.wdtView->layout()->addWidget(FViewWidget->instance());
	connect(FViewWidget->instance(),SIGNAL(viewContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)),
		SLOT(onViewWidgetContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)));

	FNoticeWidget = FMessageWidgets->newNoticeWidget(AStreamJid,AContactJid);
	ui.wdtNotice->setLayout(new QVBoxLayout);
	ui.wdtNotice->layout()->setMargin(0);
	ui.wdtNotice->layout()->addWidget(FNoticeWidget->instance());
	ui.wdtNotice->setVisible(false);
	connect(FNoticeWidget->instance(),SIGNAL(noticeActivated(int)),SLOT(onNoticeActivated(int)));

	FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid);
	ui.wdtEdit->setLayout(new QVBoxLayout);
	ui.wdtEdit->layout()->setMargin(0);
	ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
	connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
	setMenuBar(FMenuBarWidget->instance());

	FToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	FToolBarWidget->toolBarChanger()->toolBar()->setIconSize(QSize(32,32));
	ui.wdtToolBar->setLayout(new QVBoxLayout);
	ui.wdtToolBar->layout()->setMargin(0);
	ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
	setStatusBar(FStatusBarWidget->instance());

	initialize();
}

ChatWindow::~ChatWindow()
{
	emit tabPageDestroyed();
	if (FTabPageNotifier)
		delete FTabPageNotifier->instance();
	delete FInfoWidget->instance();
	delete FViewWidget->instance();
	delete FNoticeWidget->instance();
	delete FEditWidget->instance();
	delete FMenuBarWidget->instance();
	delete FToolBarWidget->instance();
	delete FStatusBarWidget->instance();
}

void ChatWindow::showTabPage()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void ChatWindow::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QString ChatWindow::tabPageId() const
{
	return "ChatWindow|"+FStreamJid.pBare()+"|"+FContactJid.pBare();
}

bool ChatWindow::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

ITabPageNotifier *ChatWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void ChatWindow::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

void ChatWindow::setContactJid(const Jid &AContactJid)
{
	if (FMessageWidgets->findChatWindow(FStreamJid,AContactJid) == NULL)
	{
		Jid befour = FContactJid;
		FContactJid = AContactJid;
		FInfoWidget->setContactJid(FContactJid);
		FViewWidget->setContactJid(FContactJid);
		FNoticeWidget->setContactJid(FContactJid);
		FEditWidget->setContactJid(FContactJid);
		emit contactJidChanged(befour);
	}
}

void ChatWindow::updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle)
{
	setWindowIcon(AIcon);
	setWindowIconText(AIconText);
	setWindowTitle(ATitle);
	emit tabPageChanged();
}

void ChatWindow::initialize()
{
	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (xmppStreams)
		{
			IXmppStream *xmppStream = xmppStreams->xmppStream(FStreamJid);
			if (xmppStream)
			{
				connect(xmppStream->instance(),SIGNAL(jidChanged(const Jid &)), SLOT(onStreamJidChanged(const Jid &)));
			}
		}
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

void ChatWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveGeometry(),"messages.chatwindow.geometry",tabPageId());
	}
}

void ChatWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		restoreGeometry(Options::fileValue("messages.chatwindow.geometry",tabPageId()).toByteArray());
	}
}

bool ChatWindow::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::KeyPress)
	{
		static QKeyEvent *sentEvent = NULL;
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
		if (sentEvent!=keyEvent && !keyEvent->text().isEmpty())
		{
			sentEvent = keyEvent;
			FEditWidget->textEdit()->setFocus();
			QCoreApplication::sendEvent(FEditWidget->textEdit(),AEvent);
			sentEvent = NULL;
			AEvent->accept();
			return true;
		}
	}
	else if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QMainWindow::event(AEvent);
}

void ChatWindow::showEvent(QShowEvent *AEvent)
{
	if (!FShownDetached && isWindow())
		loadWindowGeometry();
	FShownDetached = isWindow();
	QMainWindow::showEvent(AEvent);
	FEditWidget->textEdit()->setFocus();
	emit tabPageActivated();
}

void ChatWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void ChatWindow::onMessageReady()
{
	emit messageReady();
}

void ChatWindow::onStreamJidChanged(const Jid &ABefour)
{
	IXmppStream *xmppStream = qobject_cast<IXmppStream *>(sender());
	if (xmppStream)
	{
		if (FStreamJid && xmppStream->streamJid())
		{
			FStreamJid = xmppStream->streamJid();
			FInfoWidget->setStreamJid(FStreamJid);
			FViewWidget->setStreamJid(FStreamJid);
			FNoticeWidget->setStreamJid(FStreamJid);
			FEditWidget->setStreamJid(FStreamJid);
			emit streamJidChanged(ABefour);
		}
		else
		{
			deleteLater();
		}
	}
}

void ChatWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_SHOWINFOWIDGET)
	{
		ui.wdtInfo->setVisible(ANode.value().toBool());
	}
}

void ChatWindow::onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu)
{
	Q_UNUSED(APosition);
	if (!ASelection.toPlainText().trimmed().isEmpty())
	{
		Action *action = new Action(AMenu);
		action->setText(tr("Quote"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_MESSAGEWIDGETS_QUOTE);
		connect(action,SIGNAL(triggered(bool)),SLOT(onViewContextQuoteActionTriggered(bool)));
		AMenu->addAction(action,AG_VWCM_MESSAGEWIDGETS_QUOTE,true);
	}
}

void ChatWindow::onViewContextQuoteActionTriggered(bool)
{
	QTextDocumentFragment fragment = viewWidget()->messageStyle()->selection(viewWidget()->styleWidget());
	if (!fragment.toPlainText().trimmed().isEmpty())
	{
		QTextEdit *editor = editWidget()->textEdit();
		editor->textCursor().beginEditBlock();
		if (!editor->textCursor().atBlockStart())
			editor->textCursor().insertText("\n");
		editor->textCursor().insertText("> ");
		editor->textCursor().insertFragment(fragment);
		editor->textCursor().insertText("\n");
		editor->textCursor().endEditBlock();
		editor->setFocus();
	}
}

void ChatWindow::onNoticeActivated(int ANoticeId)
{
	ui.wdtNotice->setVisible(ANoticeId > 0);
}

