#include "metatabwindow.h"

#define ADR_ITEM_JID     Action::DR_Parametr1

MetaTabWindow::MetaTabWindow(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METATABWINDOW);

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;
	FMessageWidgets = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	initialize(APluginManager);

	FTabPageNotifier = NULL;
	FShownDetached = false;

	FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);
	FToolBarChanger->setSeparatorsVisible(false);
	FToolBarChanger->toolBar()->setIconSize(QSize(32,32));

	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const Jid &)),SLOT(onMetaPresenceChanged(const Jid &)));
	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(ui.stwWidgets,SIGNAL(currentChanged(int)),SLOT(onCurrentWidgetChanged(int)));

	updateItemButtons(FMetaRoster->metaContact(FMetaId).items);
	updateWindow();
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

ToolBarChanger *MetaTabWindow::toolBarChanger() const
{
	return FToolBarChanger;
}

ITabPage *MetaTabWindow::itemPage(const Jid &AItemJid) const
{
	return FItemTabPages.value(AItemJid.pBare());
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

			disconnect(curTabPage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
			disconnect(curTabPage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
			if (curTabPage->tabPageNotifier())
			{
				disconnect(curTabPage->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),this,SLOT(onTabPageNotifierNotifyInserted(int)));
				disconnect(curTabPage->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),this,SLOT(onTabPageNotifierNotifyRemoved(int)));
			}
			disconnect(curTabPage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
			FItemTabPages.remove(AItemJid);
			ui.stwWidgets->removeWidget(curTabPage->instance());
			curTabPage->instance()->deleteLater();
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
			{
				connect(APage->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),SLOT(onTabPageNotifierNotifyInserted(int)));
				connect(APage->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTabPageNotifierNotifyRemoved(int)));
			}
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
		{
			int itemShow = 0;
			Jid itemJid = AItemJid;
			QList<IPresenceItem> pitems = FMetaRoster->itemPresences(AItemJid);
			foreach(IPresenceItem pitem, pitems)
			{
				if (itemShow==0 || itemShow > pitem.show)
				{
					itemShow = pitem.show;
					itemJid = pitem.itemJid;
				}
			}
			emit itemPageRequested(itemJid);
		}

		ITabPage *page = FItemTabPages.value(AItemJid);
		if (page != NULL)
		{
			FItemButtons[AItemJid]->setChecked(true);
			ui.stwWidgets->setCurrentWidget(page->instance());
		}
	}
}

void MetaTabWindow::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	
	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
}

Jid MetaTabWindow::firstItemJid() const
{
	Jid firtItem;
	if (FItemButtons.count() == 1)
	{
		firtItem = FItemButtons.constBegin().key();
	}
	else foreach(QAction *handle, FToolBarChanger->toolBar()->actions())
	{
		firtItem = FItemButtons.key(qobject_cast<QToolButton *>(FToolBarChanger->handleWidget(handle)));
		if (firtItem.isValid())
			break;
	}
	return firtItem;
}

void MetaTabWindow::updateWindow()
{
	IMetaContact contact = FMetaRoster->metaContact(FMetaId);
	IPresenceItem pitem = FMetaRoster->metaPresence(FMetaId);

	QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(FMetaId,pitem.show,SUBSCRIPTION_BOTH,false) : QIcon();
	QString name = !contact.name.isEmpty() ? contact.name : contact.id.bare();
	QString show = FStatusChanger!=NULL ? FStatusChanger->nameByShow(pitem.show) : QString::null;
	QString title = name + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);

	setWindowIcon(icon);
	setWindowIconText(name);
	setWindowTitle(title);
	FTabPageToolTip = show;

	emit tabPageChanged();
}

void MetaTabWindow::updateItemButton(const Jid &AItemJid)
{
	if (FItemButtons.contains(AItemJid))
	{
		Action *action = qobject_cast<Action *>(FItemButtons.value(AItemJid)->defaultAction());
		if (action)
		{
			ITabPage *page = itemPage(AItemJid);
			IMetaItemDescriptor descriptor = FMetaContacts->itemDescriptor(AItemJid);
			int notifyCount = page && page->tabPageNotifier() ? page->tabPageNotifier()->notifies().count() : 0;
			if (notifyCount > 0)
				action->setText(QString("%1 (%2)").arg(FMetaContacts->itemHint(AItemJid)).arg(notifyCount));
			else
				action->setText(FMetaContacts->itemHint(AItemJid));
			action->setIcon(RSR_STORAGE_MENUICONS,descriptor.icon,1);
		}
	}
}

void MetaTabWindow::updateItemButtons(const QSet<Jid> &AItems)
{
	QSet<Jid> curItems = FItemButtons.keys().toSet();
	QSet<Jid> newItems = AItems - curItems;
	QSet<Jid> oldItems = curItems - AItems;

	foreach(Jid itemJid, newItems)
	{
		IMetaItemDescriptor descriptor = FMetaContacts->itemDescriptor(itemJid);

		Action *action = new Action(FToolBarChanger->toolBar());
		action->setData(ADR_ITEM_JID,itemJid.pBare());
		connect(action,SIGNAL(triggered(bool)),SLOT(onItemButtonActionTriggered(bool)));
		
		QToolButton *button = FToolBarChanger->insertAction(action,descriptor.pageOrder);
		button->setCheckable(true);
		button->setAutoExclusive(true);
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
		updateItemButton(itemJid);
	}
}

void MetaTabWindow::removeTabPageNotifies()
{
	if (FTabPageNotifier)
	{
		foreach(int notifyId, FTabPageNotifies.values())
			FTabPageNotifier->removeNotify(notifyId);
	}
	FTabPageNotifies.clear();
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
		setCurrentItem(firstItemJid());
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
		connect(page->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),SLOT(onTabPageNotifierNotifyInserted(int)));
		connect(page->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTabPageNotifierNotifyRemoved(int)));
	}
}

void MetaTabWindow::onTabPageNotifierNotifyInserted(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	Jid itemJid = notifier!=NULL ? FItemTabPages.key(notifier->tabPage()) : Jid::null;
	if (FTabPageNotifier && itemJid.isValid() && (!isActive() || currentItem()==itemJid))
	{
		ITabPageNotify notify = notifier->notifyById(ANotifyId);
		int notifyId = FTabPageNotifier->insertNotify(notify);
		FTabPageNotifies.insert(ANotifyId,notifyId);
	}
	updateItemButton(itemJid);
}

void MetaTabWindow::onTabPageNotifierNotifyRemoved(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	Jid itemJid = notifier!=NULL ? FItemTabPages.key(notifier->tabPage()) : Jid::null;
	if (FTabPageNotifier && FTabPageNotifies.contains(ANotifyId))
	{
		int notifyId = FTabPageNotifies.take(ANotifyId);
		FTabPageNotifier->removeNotify(notifyId);
	}
	updateItemButton(itemJid);
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

void MetaTabWindow::onMetaPresenceChanged(const Jid &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		updateWindow();
	}
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
