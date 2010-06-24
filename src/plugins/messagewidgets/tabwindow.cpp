#include "tabwindow.h"

#include <QCursor>
#include <QMessageBox>
#include <QInputDialog>

#define ADR_TAB_INDEX               Action::DR_Parametr1
#define ADR_CLOSE_OTHER             Action::DR_Parametr2
#define ADR_OPEN_LAST               Action::DR_Parametr3
#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MESSAGEWIDGETS_TABWINDOW);

	FWindowId = AWindowId;
	FMessageWidgets = AMessageWidgets;

	initialize();
	loadWindowState();

	FWindowMenu = new Menu(this);
	createActions();
	addActions(FWindowMenu->actions());

	connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
	connect(ui.twtTabs,SIGNAL(tabMenuRequested(int)),SLOT(onTabMenuRequested(int)));
	connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));

	connect(FMessageWidgets->instance(),SIGNAL(tabWindowAppended(const QUuid &, const QString &)),
		SLOT(onTabWindowAppended(const QUuid &, const QString &)));
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowNameChanged(const QUuid &, const QString &)),
		SLOT(onTabWindowNameChanged(const QUuid &, const QString &)));
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowDeleted(const QUuid &)),SLOT(onTabWindowDeleted(const QUuid &)));
}

TabWindow::~TabWindow()
{
	clear();
	saveWindowState();
	emit windowDestroyed();
}

void TabWindow::showWindow()
{
	isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
	WidgetManager::raiseWidget(this);
}

QUuid TabWindow::windowId() const
{
	return FWindowId;
}

QString TabWindow::windowName() const
{
	return FMessageWidgets->tabWindowName(FWindowId);
}

Menu *TabWindow::windowMenu() const
{
	return FWindowMenu;
}

void TabWindow::addTabPage(ITabPage *APage)
{
	if (ui.twtTabs->indexOf(APage->instance()) < 0)
	{
		int index = ui.twtTabs->addTab(APage->instance(),APage->instance()->windowTitle());
		connect(APage->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
		connect(APage->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
		connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
		if (APage->tabPageNotifier())
			connect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		connect(APage->instance(),SIGNAL(tabPageNotifierChanged()),SLOT(onTabPageNotifierChanged()));
		updateTab(index);
		updateWindow();
		emit tabPageAdded(APage);
	}
}

bool TabWindow::hasTabPage(ITabPage *APage) const
{
	return ui.twtTabs->indexOf(APage->instance()) >= 0;
}

ITabPage *TabWindow::currentTabPage() const
{
	return qobject_cast<ITabPage *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentTabPage(ITabPage *APage)
{
	ui.twtTabs->setCurrentWidget(APage->instance());
}

void TabWindow::detachTabPage(ITabPage *APage)
{
	removeTabPage(APage);
	APage->instance()->show();
	if (APage->instance()->x()<=0 || APage->instance()->y()<0)
		APage->instance()->move(0,0);
	emit tabPageDetached(APage);
}

void TabWindow::removeTabPage(ITabPage *APage)
{
	int index = ui.twtTabs->indexOf(APage->instance());
	if (index >=0)
	{
		ui.twtTabs->removeTab(index);
		APage->instance()->close();
		APage->instance()->setParent(NULL);
		disconnect(APage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
		disconnect(APage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
		disconnect(APage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
		disconnect(APage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
		if (APage->tabPageNotifier())
			disconnect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		disconnect(APage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
		FLastClosedTab = APage->tabPageId();
		emit tabPageRemoved(APage);
		if (ui.twtTabs->count() == 0)
			close();
	}
}

void TabWindow::clear()
{
	while (ui.twtTabs->count() > 0)
	{
		ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(0));
		if (page)
			removeTabPage(page);
		else
			ui.twtTabs->removeTab(0);
	}
}

void TabWindow::initialize()
{
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

void TabWindow::createActions()
{
	FNextTab = new Action(FWindowMenu);
	FNextTab->setText(tr("Next Tab"));
	FNextTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+Tab") << tr("Ctrl+PgDown"));
	FWindowMenu->addAction(FNextTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FNextTab,SIGNAL(triggered()),ui.twtTabs,SLOT(showNextTab()));

	FPrevTab = new Action(FWindowMenu);
	FPrevTab->setText(tr("Prev. Tab"));
	FPrevTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+Shift+Tab") << tr("Ctrl+PgUp"));
	FWindowMenu->addAction(FPrevTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FPrevTab,SIGNAL(triggered()),ui.twtTabs,SLOT(showPrevTab()));

	FCloseTab = new Action(FWindowMenu);
	FCloseTab->setText(tr("Close Tab"));
	FCloseTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+W") << tr("Ctrl+F4"));
	FWindowMenu->addAction(FCloseTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseTab,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FCloseAllTabs = new Action(FWindowMenu);
	FCloseAllTabs->setText(tr("Close All Tabs"));
	FCloseAllTabs->setShortcut(tr("Ctrl+Shift+W"));
	FWindowMenu->addAction(FCloseAllTabs,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseAllTabs,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

/*
	FDetachWindow = new Action(FWindowMenu);
	FDetachWindow->setText(tr("Detach to Separate Window"));
	FWindowMenu->addAction(FDetachWindow,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FDetachWindow,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FJoinMenu = new Menu(FWindowMenu);
	FJoinMenu->setTitle(tr("Join to"));
	FWindowMenu->addAction(FJoinMenu->menuAction(),AG_MWTW_MWIDGETS_TAB_ACTIONS);
	foreach(QUuid windowId,FMessageWidgets->tabWindowList())
		if (windowId!=FWindowId)
			onTabWindowAppended(windowId, FMessageWidgets->tabWindowName(windowId));

	FNewTab = new Action(FJoinMenu);
	FNewTab->setText(tr("New Tab Window"));
	FJoinMenu->addAction(FNewTab,AG_DEFAULT+1);
	connect(FNewTab,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FShowCloseButtons = new Action(FWindowMenu);
	FShowCloseButtons->setText(tr("Tabs Closable"));
	FShowCloseButtons->setCheckable(true);
	FShowCloseButtons->setChecked(ui.twtTabs->tabsClosable());
	FWindowMenu->addAction(FShowCloseButtons,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FShowCloseButtons,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FSetAsDefault = new Action(FWindowMenu);
	FSetAsDefault->setText(tr("Use as Default Tab Window"));
	FSetAsDefault->setCheckable(true);
	FWindowMenu->addAction(FSetAsDefault,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FSetAsDefault,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FRenameWindow = new Action(FWindowMenu);
	FRenameWindow->setText(tr("Rename Tab Window"));
	FWindowMenu->addAction(FRenameWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FRenameWindow,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));
*/
	FCloseWindow = new Action(FWindowMenu);
	FCloseWindow->setText(tr("Close Window"));
	FCloseWindow->setShortcuts(QList<QKeySequence>() << tr("Esc") << tr("Alt+F4"));
	FWindowMenu->addAction(FCloseWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FCloseWindow,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));
/*
	FDeleteWindow = new Action(FWindowMenu);
	FDeleteWindow->setText(tr("Delete Tab Window"));
	FWindowMenu->addAction(FDeleteWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FDeleteWindow,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));
*/

	onOptionsChanged(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT));
}

void TabWindow::saveWindowState()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		Options::setFileValue(saveGeometry(),"messages.tabwindows.window.geometry",FWindowId.toString());
		Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).setValue(ui.twtTabs->tabsClosable(),"tabs-closable");
	}
}

void TabWindow::loadWindowState()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		restoreGeometry(Options::fileValue("messages.tabwindows.window.geometry",FWindowId.toString()).toByteArray());
		ui.twtTabs->setTabsClosable(Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).value("tabs-closable").toBool());
	}
}

void TabWindow::updateWindow()
{
	QWidget *widget = ui.twtTabs->currentWidget();
	if (widget)
	{
		setWindowIcon(widget->windowIcon());
		setWindowTitle(widget->windowTitle());
		emit windowChanged();
	}
}

void TabWindow::updateTab(int AIndex)
{
	QWidget *widget = ui.twtTabs->widget(AIndex);
	if (widget)
	{
		ui.twtTabs->setTabIcon(AIndex,widget->windowIcon());
		ui.twtTabs->setTabText(AIndex,widget->windowIconText());
		ui.twtTabs->setTabToolTip(AIndex,widget->toolTip());
	}
}

void TabWindow::onTabChanged(int AIndex)
{
	Q_UNUSED(AIndex);
	updateWindow();
	emit currentTabPageChanged(currentTabPage());
}

void TabWindow::onTabMenuRequested(int AIndex)
{
	Menu *tabMenu = new Menu(this);
	tabMenu->setAttribute(Qt::WA_DeleteOnClose, true);

	Action *action = new Action(tabMenu);
	action->setText(tr("Close Tab"));
	action->setData(ADR_TAB_INDEX, AIndex);
	action->setData(ADR_CLOSE_OTHER, false);
	connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
	tabMenu->addAction(action);

	action = new Action(tabMenu);
	action->setText(tr("Close Other Tabs"));
	action->setData(ADR_TAB_INDEX, AIndex);
	action->setData(ADR_CLOSE_OTHER, true);
	action->setEnabled(ui.twtTabs->count()>1);
	connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
	tabMenu->addAction(action);

	action = new Action(tabMenu);
	action->setText(tr("Open Closed Tab"));
	action->setData(ADR_OPEN_LAST, true);
	action->setEnabled(!FLastClosedTab.isEmpty());
	connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
	tabMenu->addAction(action,AG_DEFAULT+1);

	tabMenu->popup(QCursor::pos());
}

void TabWindow::onTabCloseRequested(int AIndex)
{
	ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(AIndex));
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageShow()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		setCurrentTabPage(page);
		showWindow();
	}
}

void TabWindow::onTabPageClose()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		int index = ui.twtTabs->indexOf(page->instance());
		updateTab(index);
		if (index == ui.twtTabs->currentIndex())
			updateWindow();
	}
}

void TabWindow::onTabPageDestroyed()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageNotifierChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page && page->tabPageNotifier()!=NULL)
	{
		connect(page->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),SLOT(onTabPageNotifierActiveNotifyChanged(int)));
	}
}

void TabWindow::onTabPageNotifierActiveNotifyChanged(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	if (notifier)
	{
		int index = ui.twtTabs->indexOf(notifier->tabPage()->instance());
		ui.twtTabs->setTabNotify(index,notifier->notifyById(ANotifyId));
	}
}

void TabWindow::onTabWindowAppended(const QUuid &AWindowId, const QString &AName)
{
	Action *action = new Action(FJoinMenu);
	action->setText(AName);
	action->setData(ADR_TABWINDOWID,AWindowId.toString());
	FJoinMenu->addAction(action,AG_DEFAULT);
	connect(action,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));
}

void TabWindow::onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName)
{
	if (AWindowId == FWindowId)
		updateWindow();

	foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
		if (AWindowId == action->data(ADR_TABWINDOWID).toString())
			action->setText(AName);
}

void TabWindow::onTabWindowDeleted(const QUuid &AWindowId)
{
	foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
		if (AWindowId == action->data(ADR_TABWINDOWID).toString())
			FJoinMenu->removeAction(action);
}

void TabWindow::onTabMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int index = action->data(ADR_TAB_INDEX).toInt();
		if (action->data(ADR_OPEN_LAST).toBool())
		{
			foreach(ITabPageHandler *handler, FMessageWidgets->tabPageHandlers())
			{
				ITabPage *page = handler->tabPageCreate(FLastClosedTab);
				if (page)
					page->showTabPage();
				break;
			}
			FLastClosedTab.clear();
		}
		else if (action->data(ADR_CLOSE_OTHER).toBool())
		{
			while (index+1 < ui.twtTabs->count())
				onTabCloseRequested(index+1);
			for (int i=0; i<index; i++)
				onTabCloseRequested(0);
		}
		else
		{
			onTabCloseRequested(index);
		}
	}
}

void TabWindow::onWindowMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FCloseTab)
	{
		removeTabPage(currentTabPage());
	}
	else if (action == FCloseAllTabs)
	{
		clear();
		close();
	}
	else if (action == FDetachWindow)
	{
		detachTabPage(currentTabPage());
	}
	else if (action == FNewTab)
	{
		QString name = QInputDialog::getText(this,tr("New Tab Window"),tr("Tab window name:"));
		if (!name.isEmpty())
		{
			ITabPage *page = currentTabPage();
			removeTabPage(page);
			ITabWindow *window = FMessageWidgets->createTabWindow(FMessageWidgets->appendTabWindow(name));
			window->addTabPage(page);
			window->showWindow();
		}
	}
	else if (action == FShowCloseButtons)
	{
		ui.twtTabs->setTabsClosable(action->isChecked());
	}
	else if (action == FSetAsDefault)
	{
		Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).setValue(true);
	}
	else if (action == FRenameWindow)
	{
		QString name = QInputDialog::getText(this,tr("Rename Tab Window"),tr("Tab window name:"),QLineEdit::Normal,FMessageWidgets->tabWindowName(FWindowId));
		if (!name.isEmpty())
			FMessageWidgets->setTabWindowName(FWindowId,name);
	}
	else if (action == FCloseWindow)
	{
		close();
	}
	else if (action == FDeleteWindow)
	{
		if (QMessageBox::question(this,tr("Delete Tab Window"),tr("Are you sure you want to delete this tab window?"),
		                          QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
		{
			FMessageWidgets->deleteTabWindow(FWindowId);
		}
	}
	//else if (FJoinMenu->groupActions(AG_DEFAULT).contains(action))
	//{
	//	ITabPage *page = currentTabPage();
	//	removeTabPage(page);

	//	ITabWindow *window = FMessageWidgets->createTabWindow(action->data(ADR_TABWINDOWID).toString());
	//	window->addTabPage(page);
	//}
}

void TabWindow::onOptionsChanged(const OptionsNode &ANode)
{
	Q_UNUSED(ANode);
	//if (ANode.path() == OPV_MESSAGES_TABWINDOWS_DEFAULT)
	//{
	//	FSetAsDefault->setChecked(FWindowId==ANode.value().toString());
	//	FDeleteWindow->setVisible(!FSetAsDefault->isChecked());
	//}
}
