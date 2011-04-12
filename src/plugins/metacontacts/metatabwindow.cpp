#include "metatabwindow.h"

#include <QStyle>
#include <QPainter>
#include <QMessageBox>
#include <QFontMetrics>
#include <QDesktopServices>
#include <QContextMenuEvent>
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>

#define ADR_ITEM_JID     Action::DR_Parametr1

MetaTabWindow::MetaTabWindow(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METATABWINDOW);
	ui.tlbToolBar->installEventFilter(this);

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
	FToolBarChanger->toolBar()->setIconSize(QSize(24,24));

	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const QString &)),SLOT(onMetaPresenceChanged(const QString &)));
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
	return "MetaTabWidget|"+FMetaRoster->streamJid().pBare()+"|"+FMetaId;
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

QString MetaTabWindow::metaId() const
{
	return FMetaId;
}

IMetaRoster *MetaTabWindow::metaRoster() const
{
	return FMetaRoster;
}

ITabPage *MetaTabWindow::itemPage(const Jid &AItemJid) const
{
	return FItemTabPages.value(AItemJid.pBare());
}

void MetaTabWindow::setItemPage(const Jid &AItemJid, ITabPage *APage)
{
	ITabPage *curTabPage = FItemTabPages.value(AItemJid);
	if (FItemButtons.contains(AItemJid) && curTabPage != APage)
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

		if (!FItemButtons.contains(currentItem()))
		{
			setCurrentItem(firstItemJid());
		}
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
				if (itemShow==0 || itemShow>pitem.show)
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
			ui.stwWidgets->setCurrentWidget(page->instance());
		}

		updateWindow();
	}
}

ITabPage *MetaTabWindow::currentPage() const
{
	return qobject_cast<ITabPage *>(ui.stwWidgets->currentWidget());
}

void MetaTabWindow::setCurrentPage(ITabPage *APAge)
{
	if (FPageActions.contains(APAge))
	{
		ui.stwWidgets->setCurrentWidget(APAge->instance());
	}
}

void MetaTabWindow::insertPage(ITabPage *APage, int AOrder, bool ACombine)
{
	if (APage && !FPageActions.contains(APage))
	{
		Action *action = new Action(FToolBarChanger->toolBar());
		action->setCheckable(true);
		connect(action,SIGNAL(triggered(bool)),SLOT(onPageActionTriggered(bool)));
		FPageActions.insert(APage,action);
		
		QToolButton *button = NULL;
		if (ACombine)
		{
			button = FPageButtons.value(FCombinedPages.value(AOrder));
			FCombinedPages.insertMulti(AOrder,APage);
		}
		if (!button)
		{
			button = new QToolButton(FToolBarChanger->toolBar());
			button->setCheckable(true);
			button->setAutoExclusive(true);
			button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			connect(button,SIGNAL(clicked(bool)),SLOT(onPageButtonClicked(bool)));

			FToolBarChanger->insertWidget(button,AOrder);
			setButtonAction(button,action);
		}
		else
		{
			Menu *menu = qobject_cast<Menu *>(button->menu());
			if (!menu)
			{
				menu = new Menu(button);
				if (FButtonAction.contains(button))
					menu->addAction(FButtonAction.value(button),AG_DEFAULT,true);
				button->setMenu(menu);
				button->setPopupMode(QToolButton::MenuButtonPopup);
			}
			menu->addAction(action,AG_DEFAULT,true);
		}
		FPageButtons.insert(APage,button);

		connectPage(APage);
		ui.stwWidgets->addWidget(APage->instance());

		emit pageInserted(APage,AOrder,ACombine);
	}
}

void MetaTabWindow::setPageIcon(ITabPage *APage, const QIcon &AIcon)
{
	Action *action = FPageActions.value(APage);
	if (action)
		action->setIcon(AIcon);
}

void MetaTabWindow::setPageName(ITabPage *APage, const QString &AName)
{
	Action *action = FPageActions.value(APage);
	if (action)
		action->setText(AName);

}

void MetaTabWindow::replacePage(ITabPage *AOldPage, ITabPage *ANewPage)
{
	if (ANewPage && FPageActions.contains(AOldPage))
	{
		Action *action = FPageActions.take(AOldPage);
		FPageActions.insert(ANewPage,action);

		QToolButton *button = FPageButtons.take(AOldPage);
		FPageButtons.insert(ANewPage,button);

		if (FCombinedPages.values().contains(AOldPage))
		{
			int order = FCombinedPages.key(AOldPage);
			FCombinedPages.remove(order,AOldPage);
			FCombinedPages.insertMulti(order,ANewPage);
		}

		disconnectPage(AOldPage);
		connectPage(ANewPage);

		bool show = (ui.stwWidgets->currentWidget() == AOldPage->instance());
		ui.stwWidgets->removeWidget(AOldPage->instance());
		ui.stwWidgets->addWidget(ANewPage->instance());

		if (show)
			ANewPage->showTabPage();

		emit pageReplaced(AOldPage,ANewPage);
	}
}

void MetaTabWindow::removePage(ITabPage *APage)
{
	if (FPageActions.contains(APage))
	{
		int order = FCombinedPages.key(APage);
		FCombinedPages.remove(order,APage);

		Action *action = FPageActions.take(APage);
		action->deleteLater();

		QToolButton *button = FPageButtons.take(APage);
		if (FCombinedPages.contains(order))
		{
			Menu *menu = qobject_cast<Menu *>(button->menu());
			if (menu)
			{
				if (menu->groupActions().count() <= 2)
				{
					menu->deleteLater();
					button->setMenu(NULL);
					button->setPopupMode(QToolButton::DelayedPopup);
				}
				else
				{
					menu->removeAction(action);
				}
			}
			if (FButtonAction.value(button) == action)
			{
				ITabPage *combinedPage = FCombinedPages.value(order);
				setButtonAction(button,FPageActions.value(combinedPage));
			}
		}
		else
		{
			FToolBarChanger->removeItem(FToolBarChanger->widgetHandle(button));
			setButtonAction(button,NULL);
			button->deleteLater();
		}

		disconnectPage(APage);
		ui.stwWidgets->removeWidget(APage->instance());

		emit pageRemoved(APage);
	}
}

ToolBarChanger *MetaTabWindow::toolBarChanger() const
{
	return FToolBarChanger;
}

void MetaTabWindow::insertTopWidget(int AOrder, QWidget *AWidget)
{
	Q_UNUSED(AOrder)
	if(AWidget != NULL)
	{
		ui.vlExtControls->addWidget(AWidget);
		emit topWidgetInserted(AOrder, AWidget);
	}
}
void MetaTabWindow::removeTopWidget(QWidget *AWidget)
{
	if(AWidget != NULL)
	{
		ui.vlExtControls->removeWidget(AWidget);
		emit topWidgetRemoved(AWidget);
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
	QMap<int, Jid> items = FMetaContacts->itemOrders(FItemButtons.keys());
	return items.constBegin().value();
}

void MetaTabWindow::updateWindow()
{
	IMetaContact contact = FMetaRoster->metaContact(FMetaId);
	IPresenceItem pitem = FMetaRoster->metaPresence(FMetaId);

	QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(FMetaId,pitem.show,SUBSCRIPTION_BOTH,false) : QIcon();
	QString name = FMetaContacts->metaContactName(contact);
	QString show = FStatusChanger!=NULL ? FStatusChanger->nameByShow(pitem.show) : QString::null;
	QString title = name + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);

	IMetaItemDescriptor descriptor = FMetaContacts->descriptorByItem(currentItem());
	if(!descriptor.name.isEmpty())
		title += QString(" - %1 (%2)").arg(descriptor.name).arg(FMetaContacts->itemHint(currentItem()));

	setWindowIcon(icon);
	setWindowIconText(name);
	setWindowTitle(title);
	FTabPageToolTip = show;

	emit tabPageChanged();
}

void MetaTabWindow::updateItemAction(const Jid &AItemJid)
{
	Action *action = FItemActions.value(AItemJid);
	if (action)
	{
		int notifyCount = itemNotifyCount(AItemJid,false);
		if (notifyCount > 0)
			action->setText(QString("%1 (%2)").arg(FMetaContacts->itemHint(AItemJid)).arg(notifyCount));
		else
			action->setText(FMetaContacts->itemHint(AItemJid));
	}
}

void MetaTabWindow::updateItemButton(const Jid &AItemJid)
{
	QToolButton *button = FItemButtons.value(AItemJid);
	if (button)
	{
		IMetaItemDescriptor descriptor = FMetaContacts->descriptorByItem(AItemJid);

		QIcon icon;
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);

		int notifyCount = itemNotifyCount(AItemJid,true);
		if (notifyCount > 0)
			button->setIcon(insertNotifyBalloon(icon,notifyCount));
		else
			button->setIcon(icon);

		Action *action = FButtonAction.value(button);
		button->setText(action ? action->text() : FMetaContacts->itemHint(AItemJid));
		button->setToolTip(button->text());
	}
}

void MetaTabWindow::updateItemButtons(const QSet<Jid> &AItems)
{
	QSet<Jid> curItems = FItemButtons.keys().toSet();
	QSet<Jid> newItems = AItems - curItems;
	QSet<Jid> oldItems = curItems - AItems;

	foreach(Jid itemJid, newItems)
	{
		IMetaItemDescriptor descriptor = FMetaContacts->descriptorByItem(itemJid);

		Action *action = new Action(FToolBarChanger->toolBar());
		action->setCheckable(true);
		action->setData(ADR_ITEM_JID,itemJid.pBare());
		QIcon icon;
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);
		action->setIcon(icon);
		connect(action,SIGNAL(triggered(bool)),SLOT(onItemActionTriggered(bool)));
		FItemActions.insert(itemJid,action);
		updateItemAction(itemJid);

		QToolButton *button = descriptor.combine ? FItemButtons.value(FCombinedItems.value(descriptor.name)) : NULL;
		if (!button)
		{
			button = new QToolButton(FToolBarChanger->toolBar());
			button->setCheckable(true);
			button->setAutoExclusive(true);
			button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			button->setIcon(action->icon());
			connect(button,SIGNAL(clicked(bool)),SLOT(onItemButtonClicked(bool)));
			FToolBarChanger->insertWidget(button,descriptor.pageOrder);
			setButtonAction(button,action);
		}
		else
		{
			Menu *menu = qobject_cast<Menu *>(button->menu());
			if (!menu)
			{
				menu = new Menu(button);
				if (FButtonAction.contains(button))
					menu->addAction(FButtonAction.value(button),AG_DEFAULT,true);
				button->setMenu(menu);
				button->setPopupMode(QToolButton::MenuButtonPopup);
			}
			menu->addAction(action,AG_DEFAULT,true);
		}
		if (descriptor.combine)
		{
			FCombinedItems.insertMulti(descriptor.name,itemJid);
		}
		FItemButtons.insert(itemJid,button);
	}

	foreach(Jid itemJid, oldItems)
	{
		QString descrName = FCombinedItems.key(itemJid);
		FCombinedItems.remove(descrName,itemJid);

		Action *action = FItemActions.take(itemJid);
		action->deleteLater();

		QToolButton *button = FItemButtons.take(itemJid);
		if (FCombinedItems.contains(descrName))
		{
			Menu *menu = qobject_cast<Menu *>(button->menu());
			if (menu)
			{
				if (menu->groupActions().count() <= 2)
				{
					menu->deleteLater();
					button->setMenu(NULL);
					button->setPopupMode(QToolButton::DelayedPopup);
				}
				else
				{
					menu->removeAction(action);
				}
			}
			if (FButtonAction.value(button) == action)
			{
				Jid combinedJid = FCombinedItems.value(descrName);
				setButtonAction(button,FItemActions.value(combinedJid));
				updateItemButton(combinedJid);
			}
		}
		else
		{
			FToolBarChanger->removeItem(FToolBarChanger->widgetHandle(button));
			setButtonAction(button,NULL);
			button->deleteLater();
		}
		setItemPage(itemJid,NULL);
	}

	curItems = curItems + newItems - oldItems;
	foreach(Jid itemJid, curItems)
	{
		updateItemAction(itemJid);
		updateItemButton(itemJid);
	}
}

void MetaTabWindow::setButtonAction(QToolButton *AButton, Action *AAction)
{
	Action *oldAction = FButtonAction.value(AButton);
	if (oldAction)
	{
		oldAction->setChecked(false);
	}
	if (AAction)
	{
		AAction->setChecked(true);
		FButtonAction.insert(AButton,AAction);
	}
	else
	{
		FButtonAction.remove(AButton);
	}
}

int MetaTabWindow::itemNotifyCount(const Jid &AItemJid, bool ACombined) const
{
	int notifyCount = 0;
	QList<Jid> itemsList = ACombined ? FItemButtons.keys(FItemButtons.value(AItemJid)) : QList<Jid>()<<AItemJid;
	foreach(Jid itemJid, itemsList)
	{
		ITabPage *page = itemPage(itemJid);
		ITabPageNotifier *notifier = page!=NULL ? page->tabPageNotifier() : NULL;
		if (notifier)
		{
			foreach(int notifyId, notifier->notifies())
				notifyCount += notifier->notifyById(notifyId).count;
		}
	}
	return notifyCount;
}

QIcon MetaTabWindow::insertNotifyBalloon(const QIcon &AIcon, int ACount) const
{
	if (ACount > 0)
	{
		QPixmap base = AIcon.pixmap(AIcon.availableSizes().value(0));
		QPainter painter(&base);

		//QPixmap balloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_METACONTACTS_NOTIFY_BALOON,1));
		QPixmap balloon(12,12);
		QRect ballonRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignTop|Qt::AlignRight,balloon.size(),base.rect());
		painter.drawPixmap(ballonRect,balloon);

		QString text = QString::number(ACount);
		QSize textSize = painter.fontMetrics().size(Qt::TextSingleLine,text);
		QRect textRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,textSize,ballonRect);
		painter.drawText(textRect,text);

		QIcon icon;
		icon.addPixmap(base);
		return icon;
	}
	return AIcon;
}

void MetaTabWindow::createItemContextMenu(const Jid &AItemJid, Menu *AMenu) const
{
	if (FItemButtons.contains(AItemJid))
	{
		IMetaContact contact = FMetaRoster->metaContact(FMetaId);
		IMetaItemDescriptor descriptor = FMetaContacts->descriptorByItem(AItemJid);

		QList<Jid> detachItems;
		foreach(Jid itemJid, contact.items)
		{
			IMetaItemDescriptor descriptor = FMetaContacts->descriptorByItem(itemJid);
			if (descriptor.detach)
				detachItems.append(itemJid);
		}

		Action *editAction = new Action(AMenu);
		editAction->setText(tr("Edit on site..."));
		editAction->setData(ADR_ITEM_JID,AItemJid.pBare());
		connect(editAction,SIGNAL(triggered(bool)),SLOT(onEditItemByAction(bool)));
		AMenu->addAction(editAction,AG_MCICM_ITEM_ACTIONS);

		Action *detachAction = new Action(AMenu);
		detachAction->setText(tr("Detach to separate contact"));
		detachAction->setIcon(RSR_STORAGE_MENUICONS,descriptor.icon);
		detachAction->setData(ADR_ITEM_JID,AItemJid.pBare());
		detachAction->setEnabled(FMetaRoster->isOpen() && detachItems.count()>1 && descriptor.detach);
		connect(detachAction,SIGNAL(triggered(bool)),SLOT(onDetachItemByAction(bool)));
		AMenu->addAction(detachAction,AG_MCICM_ITEM_ACTIONS);

		Action *deleteAction = new Action(AMenu);
		deleteAction->setText(tr("Delete"));
		deleteAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
		deleteAction->setData(ADR_ITEM_JID,AItemJid.pBare());
		deleteAction->setEnabled(FMetaRoster->isOpen());
		connect(deleteAction,SIGNAL(triggered(bool)),SLOT(onDeleteItemByAction(bool)));
		AMenu->addAction(deleteAction,AG_MCICM_ITEM_ACTIONS);
	}
}

void MetaTabWindow::connectPage(ITabPage *APage)
{
	if (APage)
	{
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
	}
}

void MetaTabWindow::disconnectPage(ITabPage *APage)
{
	if (APage)
	{
		disconnect(APage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
		disconnect(APage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
		disconnect(APage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
		disconnect(APage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
		if (APage->tabPageNotifier())
		{
			disconnect(APage->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),this,SLOT(onTabPageNotifierNotifyInserted(int)));
			disconnect(APage->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),this,SLOT(onTabPageNotifierNotifyRemoved(int)));
		}
		disconnect(APage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
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

bool MetaTabWindow::eventFilter(QObject * AObject, QEvent * AEvent)
{
	if (AObject == ui.tlbToolBar)
	{
		if (AEvent->type() == QEvent::Paint)
		{
			QToolButton * button = FItemButtons.value(currentItem(), NULL);
			if (button)
			{
				QPainter p(ui.tlbToolBar);
				QSize sz = ui.tlbToolBar->size();
				int buttonCenter = button->width() / 2 + button->geometry().left();
				QImage triangle = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MESSAGEWIDGETS_TABWINDOW_TRIANGLE);
				p.drawImage(buttonCenter - triangle.width() / 2, sz.height() - triangle.height(), triangle);
				QRect targetRect(0, sz.height() - triangle.height(), buttonCenter - triangle.width() / 2, triangle.height());
				QRect sourceRect(0, 0, 1, triangle.height());
				p.drawImage(targetRect, triangle, sourceRect);
				targetRect = QRect(buttonCenter + triangle.width() / 2, sz.height() - triangle.height(), sz.width() - buttonCenter - triangle.width() / 2, triangle.height());
				sourceRect = QRect(triangle.width() - 1, 0, 1, triangle.height());
				p.drawImage(targetRect, triangle, sourceRect);
				p.end();
			}
		}
	}
	return QMainWindow::eventFilter(AObject, AEvent);
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

void MetaTabWindow::contextMenuEvent(QContextMenuEvent *AEvent)
{
	QAction *handle = ui.tlbToolBar->actionAt(ui.tlbToolBar->mapFromGlobal(AEvent->globalPos()));
	Action *action = FButtonAction.value(handle!=NULL ? qobject_cast<QToolButton *>(FToolBarChanger->handleWidget(handle)) : NULL);
	Jid itemJid = action!=NULL ? action->data(ADR_ITEM_JID).toString() : Jid::null;
	if (itemJid.isValid())
	{
		Menu *menu = new Menu(this);
		createItemContextMenu(itemJid,menu);
		emit itemContextMenuRequested(itemJid,menu);

		if (menu && !menu->isEmpty())
			menu->exec(AEvent->globalPos());

		delete menu;
	}
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
	updateItemAction(itemJid);
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
	updateItemAction(itemJid);
	updateItemButton(itemJid);
}

void MetaTabWindow::onEditItemByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QDesktopServices::openUrl(QUrl("http://id.rambler.ru"));
	}
}

void MetaTabWindow::onDetachItemByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		FMetaRoster->detachContactItem(FMetaId,action->data(ADR_ITEM_JID).toString());
	}
}

void MetaTabWindow::onDeleteItemByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid itemJid = action->data(ADR_ITEM_JID).toString();
		QString message = tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(FMetaContacts->itemHint(itemJid));
		if (QMessageBox::question(NULL,tr("Remove contact"),message,QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			FMetaRoster->deleteContactItem(FMetaId,action->data(ADR_ITEM_JID).toString());
	}
}

void MetaTabWindow::onItemButtonClicked(bool)
{
	QToolButton *button = qobject_cast<QToolButton *>(sender());
	Action *action = FButtonAction.value(button);
	if (button && action)
	{
		action->trigger();
		button->setChecked(true);
	}
}

void MetaTabWindow::onItemActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		action->setChecked(true);
		setCurrentItem(action->data(ADR_ITEM_JID).toString());
	}
}

void MetaTabWindow::onCurrentWidgetChanged(int AIndex)
{
	ITabPage *page = qobject_cast<ITabPage *>(ui.stwWidgets->widget(AIndex));
	if (page)
	{
		Jid itemJid = FItemTabPages.key(page);
		QToolButton *button = FItemButtons.value(itemJid);
		setButtonAction(button,FItemActions.value(itemJid));
		updateItemButton(itemJid);
		button->setChecked(true);

		emit currentItemChanged(itemJid);
	}
	ui.tlbToolBar->repaint();
}

void MetaTabWindow::onMetaPresenceChanged(const QString &AMetaId)
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

void MetaTabWindow::onPageButtonClicked(bool)
{
	QToolButton *button = qobject_cast<QToolButton *>(sender());
	Action *action = FButtonAction.value(button);
	if (button && action)
	{
		action->trigger();
		button->setChecked(true);
	}
}

void MetaTabWindow::onPageActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		action->setChecked(true);
		setCurrentPage(FPageActions.key(action));
	}
}
