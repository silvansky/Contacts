#include "messagewidgets.h"

#include <QTextBlock>
#include <QClipboard>
#include <QDesktopServices>

#define ADR_CONTEXT_DATA							Action::DR_Parametr1

MessageWidgets::MessageWidgets()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FTrayManager = NULL;
	FOptionsManager = NULL;
}

MessageWidgets::~MessageWidgets()
{

}

void MessageWidgets::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Widgets Manager");
	APluginInfo->description = tr("Allows other modules to use standard widgets for messaging");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
}

bool MessageWidgets::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
				SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
			connect(FTrayManager->contextMenu(),SIGNAL(aboutToShow()),SLOT(onTrayContextMenuAboutToShow()));
		}
	}
	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin*>(plugin->instance());
		if (FMainWindowPlugin)
		{
//			Action * action = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
//			action->setText(tr("Mass send"));
//			FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action);
//			connect(action, SIGNAL(triggered()), SLOT(onMassSend()));
		}
	}
	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FMessageArchiver = qobject_cast<IMessageArchiver*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin*>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool MessageWidgets::initObjects()
{
	insertViewUrlHandler(this,VUHO_MESSAGEWIDGETS_DEFAULT);
	return true;
}

bool MessageWidgets::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MESSAGES_EDITORAUTORESIZE,true);
	Options::setDefaultValue(OPV_MESSAGES_SHOWINFOWIDGET,false);
	Options::setDefaultValue(OPV_MESSAGES_LASTTABPAGESCOUNT,10);
	Options::setDefaultValue(OPV_MESSAGES_EDITORMINIMUMLINES,2);
	Options::setDefaultValue(OPV_MESSAGES_EDITORSENDKEY,QKeySequence(Qt::Key_Return));
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOWS_ENABLE,true);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_NAME,tr("Tab Window"));
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_TABSCLOSABLE,true);

	if (FOptionsManager)
	{
		FOptionsManager->insertServerOption(OPV_MESSAGES_EDITORSENDKEY);

		IOptionsDialogNode dnode = { ONO_MESSAGES, OPN_MESSAGES, tr("Messages"), MNI_CHAT_MHANDLER_MESSAGE };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> MessageWidgets::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_MESSAGES)
	{
		//widgets.insertMulti(OWO_MESSAGES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE),tr("Enable tab windows"),AParent));
		//widgets.insertMulti(OWO_MESSAGES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_SHOWSTATUS),tr("Show status changes in chat windows"),AParent));
		//widgets.insertMulti(OWO_MESSAGES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_EDITORAUTORESIZE),tr("Auto resize input field"),AParent));
		//widgets.insertMulti(OWO_MESSAGES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_SHOWINFOWIDGET),tr("Show contact information in chat windows"),AParent));
		widgets.insertMulti(OWO_MESSAGES, FOptionsManager->optionsHeaderWidget(QString::null,tr("Select the method of sending messages"),AParent));
		widgets.insertMulti(OWO_MESSAGES, new MessengerOptions(AParent));
	}
	return widgets;
}

bool MessageWidgets::viewUrlOpen(IViewWidget* APage, const QUrl &AUrl, int AOrder)
{
	Q_UNUSED(APage);
	Q_UNUSED(AOrder);
	return QDesktopServices::openUrl(AUrl);
}

IInfoWidget *MessageWidgets::newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
	IInfoWidget *widget = new InfoWidget(this,AStreamJid,AContactJid);
	FCleanupHandler.add(widget->instance());
	emit infoWidgetCreated(widget);
	return widget;
}

IViewWidget *MessageWidgets::newViewWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
	IViewWidget *widget = new ViewWidget(this,AStreamJid,AContactJid);
	connect(widget->instance(),SIGNAL(viewContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)),
		SLOT(onViewWidgetContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)));
	connect(widget->instance(),SIGNAL(urlClicked(const QUrl &)),SLOT(onViewWidgetUrlClicked(const QUrl &)));
	FCleanupHandler.add(widget->instance());
	emit viewWidgetCreated(widget);
	return widget;
}

IChatNoticeWidget * MessageWidgets::newNoticeWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatNoticeWidget *widget = new ChatNoticeWidget(this,AStreamJid,AContactJid);
	FCleanupHandler.add(widget->instance());
	emit noticeWidgetCreated(widget);
	return widget;
}

IEditWidget *MessageWidgets::newEditWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
	IEditWidget *widget = new EditWidget(this,AStreamJid,AContactJid);
	FCleanupHandler.add(widget->instance());
	emit editWidgetCreated(widget);
	return widget;
}

IReceiversWidget *MessageWidgets::newReceiversWidget(const Jid &AStreamJid)
{
	IReceiversWidget *widget = new ReceiversWidget(this,AStreamJid,FVCardPlugin);
	FCleanupHandler.add(widget->instance());
	emit receiversWidgetCreated(widget);
	return widget;
}

IMenuBarWidget *MessageWidgets::newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	IMenuBarWidget *widget = new MenuBarWidget(AInfo,AView,AEdit,AReceivers);
	FCleanupHandler.add(widget->instance());
	emit menuBarWidgetCreated(widget);
	return widget;
}

IToolBarWidget *MessageWidgets::newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	IToolBarWidget *widget = new ToolBarWidget(AInfo,AView,AEdit,AReceivers);
	FCleanupHandler.add(widget->instance());
	emit toolBarWidgetCreated(widget);
	return widget;
}

IStatusBarWidget *MessageWidgets::newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	IStatusBarWidget *widget = new StatusBarWidget(AInfo,AView,AEdit,AReceivers);
	FCleanupHandler.add(widget->instance());
	emit statusBarWidgetCreated(widget);
	return widget;
}

ITabPageNotifier *MessageWidgets::newTabPageNotifier(ITabPage *ATabPage)
{
	ITabPageNotifier *notifier = new TabPageNotifier(ATabPage);
	FCleanupHandler.add(notifier->instance());
	emit tabPageNotifierCreated(notifier);
	return notifier;
}

QList<IMessageWindow *> MessageWidgets::messageWindows() const
{
	return FMessageWindows;
}

IMessageWindow *MessageWidgets::newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
	IMessageWindow *window = findMessageWindow(AStreamJid, AContactJid);
	if (!window)
	{
		window = new MessageWindow(this, AStreamJid, AContactJid, AMode);
		FMessageWindows.append(window);
		connect(window->instance(), SIGNAL(tabPageDestroyed()), SLOT(onMessageWindowDestroyed()));
		FCleanupHandler.add(window->instance());
		emit messageWindowCreated(window);
		return window;
	}
	return NULL;
}

IMessageWindow *MessageWidgets::findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IMessageWindow *window,FMessageWindows)
		if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
			return window;
	return NULL;
}

QList<IMassSendDialog*> MessageWidgets::massSendDialogs() const
{
	return FMassSendDialogs;
}

IMassSendDialog * MessageWidgets::newMassSendDialog(const Jid & AStreamJid)
{
	IMassSendDialog * dlg = findMassSendDialog(AStreamJid);
	if (!dlg)
	{
		dlg = new MassSendDialog(this, AStreamJid);
		FMassSendDialogs.append(dlg);
		FCleanupHandler.add(dlg->instance());
		emit massSendDialogCreated(dlg);
	}
	return dlg;
}

IMassSendDialog * MessageWidgets::findMassSendDialog(const Jid & AStreamJid)
{
	foreach (IMassSendDialog* dialog, FMassSendDialogs)
		if (dialog->streamJid() == AStreamJid)
			return dialog;
	return 0;
}

QList<IChatWindow *> MessageWidgets::chatWindows() const
{
	return FChatWindows;
}

IChatWindow *MessageWidgets::newChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = findChatWindow(AStreamJid,AContactJid);
	if (!window)
	{
		window = new ChatWindow(this,AStreamJid,AContactJid);
		FChatWindows.append(window);
		connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onChatWindowDestroyed()));
		FCleanupHandler.add(window->instance());
		emit chatWindowCreated(window);
		return window;
	}
	return NULL;
}

IChatWindow *MessageWidgets::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IChatWindow *window,FChatWindows)
		if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
			return window;
	return NULL;
}

QList<QUuid> MessageWidgets::tabWindowList() const
{
	QList<QUuid> list;
	foreach(QString tabWindowId, Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).childNSpaces("window"))
		list.append(tabWindowId);
	return list;
}

QUuid MessageWidgets::appendTabWindow(const QString &AName)
{
	QUuid id = QUuid::createUuid();
	QString name = AName;
	if (name.isEmpty())
	{
		QList<QString> names;
		foreach(QString tabWindowId, Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).childNSpaces("window"))
			names.append(Options::node(OPV_MESSAGES_TABWINDOW_ITEM,tabWindowId).value().toString());

		int i = 0;
		do
		{
			i++;
			name = tr("Tab Window %1").arg(i);
		} while (names.contains(name));
	}
	Options::node(OPV_MESSAGES_TABWINDOW_ITEM,id.toString()).setValue(name,"name");
	emit tabWindowAppended(id,name);
	return id;
}

void MessageWidgets::deleteTabWindow(const QUuid &AWindowId)
{
	if (AWindowId!=Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString() && tabWindowList().contains(AWindowId))
	{
		ITabWindow *window = findTabWindow(AWindowId);
		if (window)
			window->instance()->deleteLater();
		Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).removeChilds("window",AWindowId.toString());
		emit tabWindowDeleted(AWindowId);
	}
}

QString MessageWidgets::tabWindowName(const QUuid &AWindowId) const
{
	if (tabWindowList().contains(AWindowId))
		return Options::node(OPV_MESSAGES_TABWINDOW_ITEM,AWindowId.toString()).value("name").toString();
	return Options::defaultValue(OPV_MESSAGES_TABWINDOW_NAME).toString();
}

void MessageWidgets::setTabWindowName(const QUuid &AWindowId, const QString &AName)
{
	if (!AName.isEmpty() && tabWindowList().contains(AWindowId))
	{
		Options::node(OPV_MESSAGES_TABWINDOW_ITEM,AWindowId.toString()).setValue(AName,"name");
		emit tabWindowNameChanged(AWindowId,AName);
	}
}

QList<ITabWindow *> MessageWidgets::tabWindows() const
{
	return FTabWindows;
}

ITabWindow *MessageWidgets::createTabWindow(const QUuid &AWindowId)
{
	ITabWindow *window = findTabWindow(AWindowId);
	if (!window)
	{
		window = new TabWindow(this,AWindowId);
		FTabWindows.append(window);
		WidgetManager::setWindowSticky(window->instance(),true);
		connect(window->instance(),SIGNAL(tabPageAdded(ITabPage *)),SLOT(onTabPageAdded(ITabPage *)));
		connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onTabWindowDestroyed()));
		emit tabWindowCreated(window);
	}
	return window;
}

ITabWindow *MessageWidgets::findTabWindow(const QUuid &AWindowId) const
{
	foreach(ITabWindow *window,FTabWindows)
		if (window->windowId() == AWindowId)
			return window;
	return NULL;
}

ITabWindow *MessageWidgets::assignTabWindowPage(ITabPage *APage)
{
	ITabWindow *window = NULL;
	if (Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE).value().toBool())
	{
		QList<QUuid> availWindows = tabWindowList();
		QUuid windowId = FTabPageWindow.value(APage->tabPageId());
		if (!availWindows.contains(windowId))
			windowId = Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString();
		if (!availWindows.contains(windowId))
			windowId = availWindows.value(0);
		ITabWindow *window = createTabWindow(windowId);
		window->addTabPage(APage);
	}
	return window;
}

QList<IViewDropHandler *> MessageWidgets::viewDropHandlers() const
{
	return FViewDropHandlers;
}

void MessageWidgets::insertViewDropHandler(IViewDropHandler *AHandler)
{
	if (AHandler && !FViewDropHandlers.contains(AHandler))
	{
		FViewDropHandlers.append(AHandler);
		emit viewDropHandlerInserted(AHandler);
	}
}

void MessageWidgets::removeViewDropHandler(IViewDropHandler *AHandler)
{
	if (FViewDropHandlers.contains(AHandler))
	{
		FViewDropHandlers.removeAll(AHandler);
		emit viewDropHandlerRemoved(AHandler);
	}
}

QMultiMap<int, IViewUrlHandler *> MessageWidgets::viewUrlHandlers() const
{
	return FViewUrlHandlers;
}

void MessageWidgets::insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder)
{
	if (AHandler && !FViewUrlHandlers.values(AOrder).contains(AHandler))
	{
		FViewUrlHandlers.insertMulti(AOrder,AHandler);
		emit viewUrlHandlerInserted(AHandler,AOrder);
	}
}

void MessageWidgets::removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder)
{
	if (FViewUrlHandlers.values(AOrder).contains(AHandler))
	{
		FViewUrlHandlers.remove(AOrder,AHandler);
		emit viewUrlHandlerRemoved(AHandler,AOrder);
	}
}

QList<ITabPageHandler *> MessageWidgets::tabPageHandlers() const
{
	return FTabPageHandlers;
}

void MessageWidgets::insertTabPageHandler(ITabPageHandler *AHandler)
{
	if (AHandler && !FTabPageHandlers.contains(AHandler))
	{
		FTabPageHandlers.append(AHandler);
		connect(AHandler->instance(),SIGNAL(tabPageCreated(ITabPage *)),SLOT(onTabPageCreated(ITabPage *)));
		emit tabPageHandlerInserted(AHandler);
	}
}

void MessageWidgets::removeTabPageHandler(ITabPageHandler *AHandler)
{
	if (FTabPageHandlers.contains(AHandler))
	{
		FTabPageHandlers.removeAll(AHandler);
		disconnect(AHandler->instance(),SIGNAL(tabPageCreated(ITabPage *)),this,SLOT(onTabPageCreated(ITabPage *)));
		emit tabPageHandlerRemoved(AHandler);
	}
}

void MessageWidgets::deleteWindows()
{
	foreach(ITabWindow *window, tabWindows())
		delete window->instance();
}

void MessageWidgets::deleteStreamWindows(const Jid &AStreamJid)
{
	QList<IChatWindow *> chatWindows = FChatWindows;
	foreach(IChatWindow *window, chatWindows)
		if (window->streamJid() == AStreamJid)
			delete window->instance();

	QList<IMessageWindow *> messageWindows = FMessageWindows;
	foreach(IMessageWindow *window, messageWindows)
		if (window->streamJid() == AStreamJid)
			delete window->instance();
}

QString MessageWidgets::selectionHref(const QTextDocumentFragment &ASelection) const
{
	QString href;

	QTextDocument doc;
	doc.setHtml(ASelection.toHtml());

	QTextBlock block = doc.firstBlock();
	while (block.isValid())
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
		{
			if (it.fragment().charFormat().isAnchor())
			{
				if (href.isNull())
					href = it.fragment().charFormat().anchorHref();
				else if (href != it.fragment().charFormat().anchorHref())
					return QString::null;
			}
			else
				return QString::null;
		}
		block = block.next();
	}

	return href;
}

QList<Action *> MessageWidgets::createLastTabPagesActions(QObject *AParent) const
{
	QList<Action *> actions;
	for (int i = 0; i<FLastPagesOrder.count(); i++)
	{
		foreach(ITabPageHandler *handler, FTabPageHandlers)
		{
			Action *action = handler->tabPageAction(FLastPagesOrder.at(i), AParent);
			if (action)
			{
				actions.append(action);
				break;
			}
		}
	}
	return actions;
}

void MessageWidgets::onViewWidgetUrlClicked(const QUrl &AUrl)
{
	IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
	if (widget)
	{
		for (QMap<int,IViewUrlHandler *>::const_iterator it = FViewUrlHandlers.constBegin(); it!=FViewUrlHandlers.constEnd(); it++)
			if (it.value()->viewUrlOpen(widget,AUrl,it.key()))
				break;
	}
}

void MessageWidgets::onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu)
{
	Q_UNUSED(APosition);
	if (!ASelection.isEmpty())
	{
		Action *copyAction = new Action(AMenu);
		copyAction->setText(tr("Copy"));
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setData(ADR_CONTEXT_DATA,ASelection.toHtml());
		connect(copyAction,SIGNAL(triggered(bool)),SLOT(onViewContextCopyActionTriggered(bool)));
		AMenu->addAction(copyAction,AG_VWCM_MESSAGEWIDGETS_COPY,true);

		QUrl href = selectionHref(ASelection);
		if (href.isValid() && !href.isEmpty())
		{
			Action *urlAction = new Action(AMenu);
			urlAction->setText(href.scheme()=="mailto" ? tr("Send mail") : tr("Open link"));
			urlAction->setData(ADR_CONTEXT_DATA,href.toString());
			connect(urlAction,SIGNAL(triggered(bool)),SLOT(onViewContextUrlActionTriggered(bool)));
			AMenu->addAction(urlAction,AG_VWCM_MESSAGEWIDGETS_URL,true);
			AMenu->setDefaultAction(urlAction);

		  Action *copyHrefAction = new Action(AMenu);
			copyHrefAction->setText(href.scheme()=="mailto" ? tr("Copy e-mail address") : tr("Copy link address"));
			copyHrefAction->setData(ADR_CONTEXT_DATA,href.toString());
			connect(copyHrefAction,SIGNAL(triggered(bool)),SLOT(onViewContextCopyActionTriggered(bool)));
			AMenu->addAction(copyHrefAction,AG_VWCM_MESSAGEWIDGETS_COPY,true);
		}

		if (!href.isValid() || href.scheme()=="mailto")
		{
			Action *searchAction = new Action(AMenu);
			if (href.scheme()=="mailto")
			{
				searchAction->setText(tr("Search on Rambler '%1'").arg(href.path()));
				searchAction->setData(ADR_CONTEXT_DATA,href.path());
			}
			else
			{
				searchAction->setText(tr("Search on Rambler"));
				searchAction->setData(ADR_CONTEXT_DATA,ASelection.toPlainText());
			}
			connect(searchAction,SIGNAL(triggered(bool)),SLOT(onViewContextSearchActionTriggered(bool)));
			AMenu->addAction(searchAction,AG_VWCM_MESSAGEWIDGETS_SEARCH,true);
		}
	}
}

void MessageWidgets::onViewContextCopyActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QMimeData *data = new QMimeData;
		data->setHtml(action->data(ADR_CONTEXT_DATA).toString());
		QApplication::clipboard()->setMimeData(data);
	}
}

void MessageWidgets::onViewContextUrlActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QDesktopServices::openUrl(action->data(ADR_CONTEXT_DATA).toString());
	}
}

void MessageWidgets::onViewContextSearchActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QUrl url = QString("http://nova.rambler.ru/search");
		url.setQueryItems(QList<QPair<QString,QString> >() << qMakePair<QString,QString>(QString("query"),action->data(ADR_CONTEXT_DATA).toString()));
		QDesktopServices::openUrl(url);
	}
}

void MessageWidgets::onMessageWindowDestroyed()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (window)
	{
		FMessageWindows.removeAt(FMessageWindows.indexOf(window));
		emit messageWindowDestroyed(window);
	}
}

void MessageWidgets::onChatWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		FChatWindows.removeAt(FChatWindows.indexOf(window));
		emit chatWindowDestroyed(window);
	}
}

void MessageWidgets::onTabPageAdded(ITabPage *APage)
{
	ITabWindow *window = qobject_cast<ITabWindow *>(sender());
	if (window)
	{
		if (window->windowId() != Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString())
			FTabPageWindow.insert(APage->tabPageId(), window->windowId());
		else
			FTabPageWindow.remove(APage->tabPageId());
	}
}

void MessageWidgets::onTabPageCreated(ITabPage *APage)
{
	connect(APage->instance(),SIGNAL(tabPageActivated()),SLOT(onTabPageActivated()));
}

void MessageWidgets::onTabPageActivated()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		if (!FLastPagesOrder.contains(page->tabPageId()))
		{
			while (FLastPagesOrder.count() >= Options::node(OPV_MESSAGES_LASTTABPAGESCOUNT).value().toInt())
			{
				QList<QDateTime> times = FLastPagesActivity.values();
				qSort(times);
				QString pageId = FLastPagesActivity.key(times.value(0));
				FLastPagesOrder.removeAll(pageId);
				FLastPagesActivity.remove(pageId);
			}
			FLastPagesOrder.append(page->tabPageId());
		}
		FLastPagesActivity[page->tabPageId()] = QDateTime::currentDateTime();
	}
}

void MessageWidgets::onTabWindowDestroyed()
{
	ITabWindow *window = qobject_cast<ITabWindow *>(sender());
	if (window)
	{
		FTabWindows.removeAt(FTabWindows.indexOf(window));
		emit tabWindowDestroyed(window);
	}
}

void MessageWidgets::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
	if (!(AAfter && AXmppStream->streamJid()))
		deleteStreamWindows(AXmppStream->streamJid());
}

void MessageWidgets::onStreamRemoved(IXmppStream *AXmppStream)
{
	deleteStreamWindows(AXmppStream->streamJid());
}

void MessageWidgets::onTrayContextMenuAboutToShow()
{
	QList<Action *> actions = createLastTabPagesActions(FTrayManager->contextMenu());
	foreach(Action *action, actions)
	{
		FTrayManager->contextMenu()->addAction(action,AG_TMTM_MESSAGEWIDGETS_LASTTABS);
		connect(FTrayManager->contextMenu(),SIGNAL(aboutToHide()),action,SLOT(deleteLater()));
	}
}

void MessageWidgets::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	Q_UNUSED(ANotifyId);
	if (AReason==QSystemTrayIcon::Trigger && !FTabPageHandlers.isEmpty())
	{
		Menu *menu = new Menu;
		menu->setAttribute(Qt::WA_DeleteOnClose, true);

		QList<Action *> actions = createLastTabPagesActions(menu);
		if (!actions.isEmpty())
		{
			Action *showAll = new Action(menu);
			showAll->setText(tr("Open All"));
			showAll->setIcon(RSR_STORAGE_MENUICONS, MNI_MESSAGEWIDGETS_LAST_OPEN_ALL);
			menu->addAction(showAll,AG_DEFAULT-1);

			foreach(Action *action, actions)
			{
				menu->addAction(action);
				connect(showAll,SIGNAL(triggered()),action,SLOT(trigger()));
			}

			menu->popup(QCursor::pos());
			menu->activateWindow();
		}
		else
		{
			delete menu;
	}
}
}

void MessageWidgets::onOptionsOpened()
{
	if (tabWindowList().isEmpty())
		appendTabWindow(tr("Main Tab Window"));

	if (!tabWindowList().contains(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString()))
		Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).setValue(tabWindowList().value(0).toString());

	QByteArray data = Options::fileValue("messages.tab-window-pages").toByteArray();
	QDataStream stream1(data);
	stream1 >> FTabPageWindow;

	data = Options::fileValue("messages.last-tab-pages-order").toByteArray();
	QDataStream stream2(data);
	stream2 >> FLastPagesOrder;

	data = Options::fileValue("messages.last-tab-pages-activity").toByteArray();
	QDataStream stream3(data);
	stream3 >> FLastPagesActivity;

}

void MessageWidgets::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream1(&data, QIODevice::WriteOnly);
	stream1 << FTabPageWindow;
	Options::setFileValue(data,"messages.tab-window-pages");

	data.clear();
	QDataStream stream2(&data, QIODevice::WriteOnly);
	stream2 << FLastPagesOrder;
	Options::setFileValue(data,"messages.last-tab-pages-order");

	data.clear();
	QDataStream stream3(&data, QIODevice::WriteOnly);
	stream3 << FLastPagesActivity;
	Options::setFileValue(data,"messages.last-tab-pages-activity");

	deleteWindows();
}

void MessageWidgets::onMassSend()
{
	IMassSendDialog * dlg = newMassSendDialog(FAccountManager->accounts().first()->xmppStream()->streamJid());
	dlg->instance()->show();
}

Q_EXPORT_PLUGIN2(plg_messagewidgets, MessageWidgets)
