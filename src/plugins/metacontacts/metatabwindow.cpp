#include "metatabwindow.h"

#define ADR_ITEM_JID     Action::DR_Parametr1

MetaTabWindow::MetaTabWindow(IMessageWidgets *AMessageWidgets, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METATABWINDOW);

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;
	FMessageWidgets = AMessageWidgets;
	FTabPageNotifier = NULL;

	FShownDetached = false;
	FPageToolBarChanger = NULL;
	FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);

	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(ui.stwWidgets,SIGNAL(currentChanged(int)),SLOT(onCurrentWidgetChanged(int)));

	updateItemButtons(FMetaRoster->metaContact(FMetaId).items);
}

MetaTabWindow::~MetaTabWindow()
{
	emit tabPageDestroyed();
	if (FTabPageNotifier)
		delete FTabPageNotifier->instance();
}

void MetaTabWindow::showTabPage()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MetaTabWindow::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

bool MetaTabWindow::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

QString MetaTabWindow::tabPageId() const
{
	return "MetaTabWidget|"+FMetaRoster->streamJid().pBare()+"|"+FMetaId.pBare();
}

QIcon MetaTabWindow::tabPageIcon() const
{
	return windowIcon();
}

QString MetaTabWindow::tabPageCaption() const
{
	return windowIconText();
}

QString MetaTabWindow::tabPageToolTip() const
{
	return FTabPageToolTip;
}

ITabPageNotifier *MetaTabWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void MetaTabWindow::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

Jid MetaTabWindow::metaId() const
{
	return FMetaId;
}

IMetaRoster *MetaTabWindow::metaRoster() const
{
	return FMetaRoster;
}

ITabPage *MetaTabWindow::itemPage(const Jid &AItemJid) const
{
	return FItemTabPages.value(AItemJid.bare());
}

void MetaTabWindow::setItemPage(const Jid &AItemJid, ITabPage *APage)
{
	ITabPage *curTabPage = FItemTabPages.value(AItemJid);
	if (curTabPage != APage)
	{
		if (curTabPage)
		{
			IChatWindow *window = qobject_cast<IChatWindow *>(curTabPage->instance());
			if (window && window->toolBarWidget())
				window->toolBarWidget()->instance()->show();

			disconnect(curTabPage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
			if (curTabPage->tabPageNotifier())
				disconnect(curTabPage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
			disconnect(curTabPage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
			FItemTabPages.remove(AItemJid);
			ui.stwWidgets->removeWidget(curTabPage->instance());
		}
		
		if (APage)
		{
			IChatWindow *window = qobject_cast<IChatWindow *>(APage->instance());
			if (window && window->toolBarWidget())
				window->toolBarWidget()->instance()->hide();

			connect(APage->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
			connect(APage->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
			connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
			connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
			if (APage->tabPageNotifier())
				connect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
			connect(APage->instance(),SIGNAL(tabPageNotifierChanged()),SLOT(onTabPageNotifierChanged()));
			FItemTabPages.insert(AItemJid,APage);
			ui.stwWidgets->addWidget(APage->instance());
		}
		
		emit itemPageChanged(AItemJid, APage);
	}
}

Jid MetaTabWindow::currentItem() const
{
	return FItemTabPages.key(qobject_cast<ITabPage *>(ui.stwWidgets->currentWidget()));
}

void MetaTabWindow::setCurrentItem(const Jid &AItemJid)
{
	if (FItemButtons.contains(AItemJid) && currentItem()!=AItemJid)
	{
		if (!FItemTabPages.contains(AItemJid))
			emit itemPageRequested(AItemJid);

		ITabPage *page = FItemTabPages.value(AItemJid);
		if (page != NULL)
		{
			IChatWindow *window = qobject_cast<IChatWindow *>(page->instance());
			if (window && window->toolBarWidget())
				setPageToolBarChanger(window->toolBarWidget()->toolBarChanger());

			FItemButtons[AItemJid]->setChecked(true);
			ui.stwWidgets->setCurrentWidget(page->instance());
			updateWindow();
		}
	}
}

void MetaTabWindow::updateWindow()
{
	ITabPage *page = itemPage(currentItem());
	if (page)
	{
		setWindowIcon(page->tabPageIcon());
		setWindowIconText(page->tabPageCaption());
		FTabPageToolTip = page->tabPageToolTip();
		setWindowTitle(page->instance()->windowTitle());
		emit tabPageChanged();
	}
}

void MetaTabWindow::updateItemButtons(const QSet<Jid> &AItems)
{
	QSet<Jid> curItems = FItemButtons.keys().toSet();
	QSet<Jid> newItems = AItems - curItems;
	QSet<Jid> oldItems = curItems - AItems;

	foreach(Jid itemJid, newItems)
	{
		Action *action = new Action(FToolBarChanger->toolBar());
		action->setData(ADR_ITEM_JID,itemJid.pBare());
		action->setText(itemJid.pBare());
		connect(action,SIGNAL(triggered(bool)),SLOT(onItemButtonActionTriggered(bool)));
		
		QToolButton *button = FToolBarChanger->insertAction(action);
		button->setCheckable(true);
		button->setAutoExclusive(true);
		button->setFixedSize(24,24);
		FItemButtons.insert(itemJid,button);
	}

	foreach(Jid itemJid, oldItems)
	{
		QToolButton *button = FItemButtons.take(itemJid);
		FToolBarChanger->removeItem(FToolBarChanger->widgetHandle(button));
		button->deleteLater();
		setItemPage(itemJid,NULL);
	}

	curItems = curItems + newItems - oldItems;
	foreach(Jid itemJid, curItems)
	{
	
	}
}

void MetaTabWindow::setPageToolBarChanger(ToolBarChanger *AChanger)
{
	if (FPageToolBarChanger)
	{
		foreach(QAction *handle, FPageToolBarActions)
		{
			FToolBarChanger->removeItem(handle);
		}
		disconnect(FPageToolBarChanger,SIGNAL(itemInserted(QAction *, QAction *, Action *, QWidget *, int)),
			this,SLOT(onChatWindowToolBarItemInserted(QAction *, QAction *, Action *, QWidget *, int)));
		disconnect(FPageToolBarChanger,SIGNAL(itemRemoved(QAction *)),this,SLOT(onChatWindowToolBarItemRemoved(QAction *)));
	}

	FPageToolBarActions.clear();
	FPageToolBarChanger = AChanger;

	if (FPageToolBarChanger)
	{
		foreach(QAction *handle, FPageToolBarChanger->groupItems())
		{
			Action *action = FPageToolBarChanger->handleAction(handle);
			if (action && !action->isSeparator())
			{
				FToolBarChanger->insertAction(action,FPageToolBarChanger->itemGroup(handle))->setPopupMode(QToolButton::InstantPopup);
				FPageToolBarActions.append(FToolBarChanger->actionHandle(action));
			}
		}
		connect(FPageToolBarChanger,SIGNAL(itemInserted(QAction *, QAction *, Action *, QWidget *, int)),
			SLOT(onChatWindowToolBarItemInserted(QAction *, QAction *, Action *, QWidget *, int)));
		connect(FPageToolBarChanger,SIGNAL(itemRemoved(QAction *)),SLOT(onChatWindowToolBarItemRemoved(QAction *)));
	}
}

void MetaTabWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"messages.metatabwidget.state",tabPageId());
		Options::setFileValue(saveGeometry(),"messages.metatabwidget.geometry",tabPageId());
	}
}

void MetaTabWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		if (!restoreGeometry(Options::fileValue("messages.metatabwidget.geometry",tabPageId()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("messages.metatabwidget.state",tabPageId()).toByteArray());
	}
}

bool MetaTabWindow::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QMainWindow::event(AEvent);
}

void MetaTabWindow::showEvent(QShowEvent *AEvent)
{
	if (!FShownDetached)
		loadWindowGeometry();
	FShownDetached = isWindow();
	if (FItemTabPages.isEmpty())
		setCurrentItem(FItemButtons.keys().value(0));
	QMainWindow::showEvent(AEvent);
	emit tabPageActivated();
}

void MetaTabWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void MetaTabWindow::onTabPageShow()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		setCurrentItem(FItemTabPages.key(page));
	showTabPage();
}

void MetaTabWindow::onTabPageClose()
{
	closeTabPage();
}

void MetaTabWindow::onTabPageChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page == itemPage(currentItem()))
		updateWindow();
}

void MetaTabWindow::onTabPageDestroyed()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		setItemPage(FItemTabPages.key(page), NULL);
}

void MetaTabWindow::onTabPageNotifierChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page && page->tabPageNotifier())
	{
		connect(page->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),SLOT(onTabPageNotifierActiveNotifyChanged(int)));
	}
}

void MetaTabWindow::onTabPageNotifierActiveNotifyChanged(int ANotifyId)
{

}

void MetaTabWindow::onItemButtonActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		setCurrentItem(action->data(ADR_ITEM_JID).toString());
	}
}

void MetaTabWindow::onCurrentWidgetChanged(int AIndex)
{
	ITabPage *page = qobject_cast<ITabPage *>(ui.stwWidgets->widget(AIndex));
	emit currentItemChanged(FItemTabPages.key(page));
}

void MetaTabWindow::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	Q_UNUSED(ABefore);
	if (AContact.id == FMetaId)
	{
		if (!AContact.items.isEmpty())
		{
			updateItemButtons(AContact.items);
			updateWindow();
		}
		else
		{
			deleteLater();
		}
	}
}

void MetaTabWindow::onChatWindowToolBarItemInserted(QAction *ABefour, QAction *AHandle, Action *AAction, QWidget *AWidget, int AGroup)
{
	Q_UNUSED(ABefour); Q_UNUSED(AHandle); Q_UNUSED(AWidget);
	ToolBarChanger *changer = qobject_cast<ToolBarChanger *>(sender());
	if (FPageToolBarChanger==changer && AAction && !AAction->isSeparator())
	{
		FToolBarChanger->insertAction(AAction,AGroup)->setPopupMode(QToolButton::InstantPopup);
		FPageToolBarActions.append(FToolBarChanger->actionHandle(AAction));
	}
}

void MetaTabWindow::onChatWindowToolBarItemRemoved(QAction *AHandle)
{
	ToolBarChanger *changer = qobject_cast<ToolBarChanger *>(sender());
	if (FPageToolBarChanger == changer)
	{
		QAction *handle = FToolBarChanger->actionHandle(FPageToolBarChanger->handleAction(AHandle));
		if (handle)
{
		FToolBarChanger->removeItem(handle);
		FPageToolBarActions.removeAll(handle);
}
	}
}
