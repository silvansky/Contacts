#include "metatabwindow.h"

#include <QStyle>
#include <QTimer>
#include <QPainter>
#include <QMessageBox>
#include <QFontMetrics>
#include <QDesktopServices>
#include <QContextMenuEvent>

#define ADR_ITEM_JID         Action::DR_Parametr1
#define ADR_DEFAULT_ICON     Action::DR_UserDefined+1

QList<int> MetaTabWindow::FPersistantList;

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
	FRosterChanger = NULL;
	initialize(APluginManager);

	FTabPageNotifier = NULL;
	FShownDetached = false;
	FLastItemJid = Options::fileValue("messages.metatabwidget.last-item",tabPageId()).toString();

	FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);
	FToolBarChanger->setSeparatorsVisible(false);
	FToolBarChanger->toolBar()->setIconSize(QSize(24,24));

	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const QString &)),SLOT(onMetaPresenceChanged(const QString &)));
	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(ui.stwWidgets,SIGNAL(currentChanged(int)),SLOT(onCurrentWidgetChanged(int)));

	createPersistantList();
	updateItemPages(FMetaRoster->metaContact(FMetaId).items);
	updateWindow();
}

MetaTabWindow::~MetaTabWindow()
{
	Options::setFileValue(FLastItemJid.pBare(),"messages.metatabwidget.last-item",tabPageId());

	foreach(QString pageId, FPageActions.keys()) {
		removePage(pageId); }

	setTabPageNotifier(NULL);

	emit tabPageDestroyed();
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

ToolBarChanger *MetaTabWindow::toolBarChanger() const
{
	return FToolBarChanger;
}

void MetaTabWindow::insertTopWidget(int AOrder, QWidget *AWidget)
{
	Q_UNUSED(AOrder);
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

QList<QString> MetaTabWindow::pages() const
{
	return FPageWidgets.keys();
}

QString MetaTabWindow::currentPage() const
{
	return widgetPage(qobject_cast<ITabPage *>(ui.stwWidgets->currentWidget()));
}

void MetaTabWindow::setCurrentPage(const QString &APageId)
{
	if (FPageActions.contains(APageId))
	{
		if (!FPageWidgets.contains(APageId))
		{
			if (FPersistantPages.values().contains(APageId))
				insertPersistantWidget(APageId);
			else
				emit pageWidgetRequested(APageId);
		}

		ITabPage *page = FPageWidgets.value(APageId);
		if (page && ui.stwWidgets->currentWidget()!=page->instance())
			ui.stwWidgets->setCurrentWidget(page->instance());
		else if (FPageButtons.contains(currentPage()))
			FPageButtons.value(currentPage())->setChecked(true);
	}
}

QString MetaTabWindow::insertPage(int AOrder, bool ACombine)
{
	QString pageId;
	// bad algorythm...
	while (pageId.isEmpty() || FPageWidgets.contains(pageId))
		pageId = QString::number(qrand());

	Action *action = new Action(FToolBarChanger->toolBar());
	action->setCheckable(true);
	connect(action,SIGNAL(triggered(bool)),SLOT(onPageActionTriggered(bool)));
	FPageActions.insert(pageId,action);

	QToolButton *button = NULL;
	if (ACombine)
	{
		button = FPageButtons.value(FCombinedPages.value(AOrder));
		FCombinedPages.insertMulti(AOrder,pageId);
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
	button->installEventFilter(this);
	FPageButtons.insert(pageId,button);

	emit pageInserted(pageId,AOrder,ACombine);

	return pageId;
}

QIcon MetaTabWindow::pageIcon(const QString &APageId) const
{
	Action *action = FPageActions.value(APageId);
	if (action)
		return action->icon();
	return QIcon();
}

void MetaTabWindow::setPageIcon(const QString &APageId, const QIcon &AIcon)
{
	Action *action = FPageActions.value(APageId);
	if (action)
	{
		action->setData(ADR_DEFAULT_ICON,AIcon);
		updatePageButton(APageId);
		emit pageChanged(APageId);
	}
}

QString MetaTabWindow::pageName(const QString &APageId) const
{
	Action *action = FPageActions.value(APageId);
	if (action)
		return action->text();
	return QString::null;
}

void MetaTabWindow::setPageName(const QString &APageId, const QString &AName)
{
	Action *action = FPageActions.value(APageId);
	if (action)
	{
		action->setText(AName);
		updatePageButton(APageId);
		emit pageChanged(APageId);
	}
}

QString MetaTabWindow::widgetPage(ITabPage *AWidget) const
{
	return FPageWidgets.key(AWidget);
}

ITabPage *MetaTabWindow::pageWidget(const QString &APageId) const
{
	return FPageWidgets.value(APageId);
}

void MetaTabWindow::setPageWidget(const QString &APageId, ITabPage *AWidget)
{
	if (FPageActions.contains(APageId))
	{
		bool show = false;

		ITabPage *oldWidget = FPageWidgets.value(APageId);
		if (oldWidget)
		{
			disconnectPageWidget(oldWidget);
			show = ui.stwWidgets->currentWidget()==oldWidget->instance();
			FPageWidgets.remove(APageId);
			ui.stwWidgets->removeWidget(oldWidget->instance());
		}

		if (AWidget)
		{
			foreach(IMetaTabWindow *window, FMetaContacts->metaTabWindows())
			{
				QString pageId = window->widgetPage(AWidget);
				if (!pageId.isEmpty())
				{
					window->setPageWidget(pageId,NULL);
					window->removePage(pageId);
				}
			}
			connectPageWidget(AWidget);
			FPageWidgets.insert(APageId,AWidget);
			ui.stwWidgets->addWidget(AWidget->instance());
		}

		emit pageChanged(APageId);

		if (AWidget && show)
			AWidget->showTabPage();

		checkCurrentPage();
	}
}

void MetaTabWindow::removePage(const QString &APageId)
{
	if (FPageActions.contains(APageId))
	{
		int order = FCombinedPages.key(APageId);
		FCombinedPages.remove(order,APageId);

		Action *action = FPageActions.take(APageId);
		action->deleteLater();

		QToolButton *button = FPageButtons.take(APageId);
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
				QString combinedPage = FCombinedPages.value(order);
				setButtonAction(button,FPageActions.value(combinedPage));
			}
		}
		else
		{
			FToolBarChanger->removeItem(FToolBarChanger->widgetHandle(button));
			setButtonAction(button,NULL);
			button->deleteLater();
		}

		ITabPage *widget = FPageWidgets.take(APageId);
		if (widget)
		{
			disconnectPageWidget(widget);
			ui.stwWidgets->removeWidget(widget->instance());
			widget->instance()->deleteLater();
		}

		emit pageRemoved(APageId);

		checkCurrentPage();
	}
}

bool MetaTabWindow::isContactPage() const
{
	return !FMetaRoster->metaContact(FMetaId).id.isEmpty();
}

Jid MetaTabWindow::currentItem() const
{
	return pageItem(currentPage());
}

void MetaTabWindow::setCurrentItem(const Jid &AItemJid)
{
	setCurrentPage(itemPage(AItemJid));
}

Jid MetaTabWindow::pageItem(const QString &APageId) const
{
	return FItemPages.key(APageId);
}

QString MetaTabWindow::itemPage(const Jid &AItemJid) const
{
	return FItemPages.value(AItemJid.pBare());
}

ITabPage *MetaTabWindow::itemWidget(const Jid &AItemJid) const
{
	return pageWidget(itemPage(AItemJid));
}

void MetaTabWindow::setItemWidget(const Jid &AItemJid, ITabPage *AWidget)
{
	ITabPage *oldWidget = itemWidget(AItemJid);
	if (FItemPages.contains(AItemJid) && oldWidget!=AWidget)
	{
		if (oldWidget)
		{
			IChatWindow *window = qobject_cast<IChatWindow *>(oldWidget->instance());
			if (window && window->toolBarWidget())
				window->toolBarWidget()->instance()->show();
			oldWidget->instance()->deleteLater();
		}

		if (AWidget)
		{
			IChatWindow *window = qobject_cast<IChatWindow *>(AWidget->instance());
			if (window && window->toolBarWidget())
				window->toolBarWidget()->instance()->hide();
		}

		setPageWidget(itemPage(AItemJid),AWidget);
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

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());
}

void MetaTabWindow::updateWindow()
{
	if (isContactPage())
	{
		IMetaContact contact = FMetaRoster->metaContact(FMetaId);
		IPresenceItem pitem = FMetaRoster->metaPresenceItem(FMetaId);

		QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(FMetaId,pitem.show,SUBSCRIPTION_BOTH,false) : QIcon();
		QString name = FMetaContacts->metaContactName(contact);
		QString show = FStatusChanger!=NULL ? FStatusChanger->nameByShow(pitem.show) : QString::null;
		QString title = name + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);

		setWindowIcon(icon);
		setWindowIconText(name);
		setWindowTitle(title);
		FTabPageToolTip = show;
	}
	else
	{
		ITabPage *widget = pageWidget(currentPage());
		setWindowIcon(widget!=NULL ? widget->tabPageIcon() : QIcon());
		setWindowIconText(widget!=NULL ? widget->tabPageCaption() : QString::null);
		setWindowTitle(widget!=NULL ? widget->instance()->windowTitle() : QString::null);
		FTabPageToolTip = widget!=NULL ? widget->tabPageToolTip() : QString::null;
	}

	emit tabPageChanged();
}

void MetaTabWindow::checkCurrentPage()
{
	if (pageWidget(currentPage()) == NULL)
	{
		if (isContactPage())
			setCurrentItem(lastItemJid());
		else
			setCurrentPage(FPageActions.keys().value(0));
	}
}

void MetaTabWindow::updatePageButton(const QString &APageId)
{
	QToolButton *button = FPageButtons.value(APageId);
	if (button)
	{
		Action *action = FButtonAction.value(button);
		QIcon icon = action->data(ADR_DEFAULT_ICON).value<QIcon>();
		button->setIcon(icon);
		button->setText(action->text());
		button->setToolTip(action->text());
	}
}

void MetaTabWindow::updatePageButtonNotify(const QString &APageId)
{
	QToolButton *button = FPageButtons.value(APageId);
	if (button)
	{
		int notifyCount = pageNotifyCount(APageId,true);
		if (notifyCount > 0)
		{
			QIcon notifyIcon = createNotifyBalloon(notifyCount);
			button->setProperty("notifyBalloon", notifyIcon);
		}
		else
		{
			button->setProperty("notifyBalloon", QVariant());
		}
		button->update();
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

int MetaTabWindow::pageNotifyCount(const QString &APageId, bool ACombined) const
{
	int notifyCount = 0;
	QList<QString> pagesList = ACombined ? FPageButtons.keys(FPageButtons.value(APageId)) : QList<QString>()<<APageId;
	foreach(QString pageId, pagesList)
	{
		ITabPage *page = pageWidget(pageId);
		ITabPageNotifier *notifier = page!=NULL ? page->tabPageNotifier() : NULL;
		if (notifier)
		{
			foreach(int notifyId, notifier->notifies())
				notifyCount += notifier->notifyById(notifyId).count;
		}
	}
	return notifyCount;
}

QIcon MetaTabWindow::createNotifyBalloon(int ACount) const
{
	QPixmap balloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_METACONTACTS_NOTIFY_BALOON, 0));
	QPainter painter(&balloon);
	QString text = QString::number(ACount);
	QSize textSize = painter.fontMetrics().size(Qt::TextSingleLine, text);
	QRect textRect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, textSize, balloon.rect());
	textRect.moveTopLeft(textRect.topLeft() + QPoint(1, -2)); // some magic numbers... may change
	painter.drawText(textRect,text);
	QIcon icon;
	icon.addPixmap(balloon);
	return icon;
}

Jid MetaTabWindow::lastItemJid() const
{
	if (!FItemPages.contains(FLastItemJid))
	{
		QMap<int, Jid> items = FMetaContacts->itemOrders(FItemPages.keys());
		return !items.isEmpty() ? items.constBegin().value() : Jid::null;
	}
	return FLastItemJid;
}

void MetaTabWindow::updateItemPages(const QSet<Jid> &AItems)
{
	QSet<Jid> curItems = FItemPages.keys().toSet();
	QSet<Jid> newItems = AItems - curItems;
	QSet<Jid> oldItems = curItems - AItems;

	foreach(Jid itemJid, newItems)
	{
		IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemJid);
		QString pageId = insertPage(descriptor.metaOrder,descriptor.combine);

		QIcon icon;
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
		icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);
		setPageIcon(pageId,icon);
		setPageName(pageId,FMetaContacts->itemHint(itemJid));

		FItemPages.insert(itemJid,pageId);
		FItemTypeCount[descriptor.metaOrder]++;

		updateItemButtonStatus(itemJid);
	}

	foreach(Jid itemJid, oldItems)
	{
		IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemJid);
		removePage(itemPage(itemJid));
		FItemPages.remove(itemJid);
		FItemTypeCount[descriptor.metaOrder]--;
	}

	updatePersistantPages();
}

void MetaTabWindow::updateItemButtonStatus(const Jid &AItemJid)
{
	QToolButton *button = FPageButtons.value(itemPage(AItemJid));
	if (button)
	{
		QList<IPresenceItem> pitems = FMetaRoster->itemPresences(AItemJid);
		if (!pitems.isEmpty() && pitems.at(0).show!=IPresence::Error)
		{
			QImage image = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_METACONTACTS_ONLINE_ICON);
			button->setProperty("statusIcon", image);
		}
		else
		{
			button->setProperty("statusIcon", QVariant());
		}
		button->update();
	}
}

void MetaTabWindow::createItemContextMenu(const Jid &AItemJid, Menu *AMenu) const
{
	if (FItemPages.contains(AItemJid))
	{
		IMetaContact contact = FMetaRoster->metaContact(FMetaId);
		IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(AItemJid);

		QList<Jid> detachItems;
		foreach(Jid itemJid, contact.items)
		{
			IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemJid);
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
		//detachAction->setIcon(RSR_STORAGE_MENUICONS,descriptor.icon);
		detachAction->setData(ADR_ITEM_JID,AItemJid.pBare());
		detachAction->setEnabled(FMetaRoster->isOpen() && detachItems.count()>1 && descriptor.detach);
		connect(detachAction,SIGNAL(triggered(bool)),SLOT(onDetachItemByAction(bool)));
		AMenu->addAction(detachAction,AG_MCICM_ITEM_ACTIONS);

		Action *deleteAction = new Action(AMenu);
		deleteAction->setText(tr("Delete"));
		//deleteAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
		deleteAction->setData(ADR_ITEM_JID,AItemJid.pBare());
		deleteAction->setEnabled(FMetaRoster->isOpen());
		connect(deleteAction,SIGNAL(triggered(bool)),SLOT(onDeleteItemByAction(bool)));
		AMenu->addAction(deleteAction,AG_MCICM_ITEM_ACTIONS);
	}
}

void MetaTabWindow::createPersistantList()
{
	static bool created = false;
	if (FRosterChanger && !created)
	{
		foreach(const IMetaItemDescriptor &descriptor, FMetaContacts->metaDescriptors())
		{
			if (descriptor.persistent && !descriptor.gateId.isEmpty())
				FPersistantList.append(descriptor.metaOrder);
		}
		created = true;
	}
}

void MetaTabWindow::updatePersistantPages()
{
	foreach(int metaOrder, FPersistantList)
	{
		if (isContactPage() && FPersistantPages.value(metaOrder).isEmpty() && FItemTypeCount.value(metaOrder)==0)
		{
			IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByOrder(metaOrder);
			QString pageId = insertPage(descriptor.metaOrder, false);

			QIcon icon;
			icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 1)), QIcon::Normal);
			icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Selected);
			icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 2)), QIcon::Active);
			icon.addPixmap(QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.icon, 3)), QIcon::Disabled);
			setPageIcon(pageId,icon);
			setPageName(pageId,tr("Add contact"));

			FPersistantPages.insert(metaOrder,pageId);
		}
		else if (!FPersistantPages.value(metaOrder).isEmpty() && FItemTypeCount.value(metaOrder)>0)
		{
			removePage(FPersistantPages.take(metaOrder));
		}
	}
}

void MetaTabWindow::insertPersistantWidget(const QString &APageId)
{
	IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByOrder(FPersistantPages.key(APageId));
	if (!descriptor.gateId.isEmpty())
	{
		AddMetaItemPage *widget = new AddMetaItemPage(FRosterChanger, this, FMetaRoster, FMetaId, descriptor);
		setPageWidget(APageId,widget);
	}
}

void MetaTabWindow::connectPageWidget(ITabPage *AWidget)
{
	if (AWidget)
	{
		connect(AWidget->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
		connect(AWidget->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
		connect(AWidget->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(AWidget->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
		if (AWidget->tabPageNotifier())
		{
			connect(AWidget->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),SLOT(onTabPageNotifierNotifyInserted(int)));
			connect(AWidget->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTabPageNotifierNotifyRemoved(int)));
		}
		connect(AWidget->instance(),SIGNAL(tabPageNotifierChanged()),SLOT(onTabPageNotifierChanged()));
	}
}

void MetaTabWindow::disconnectPageWidget(ITabPage *AWidget)
{
	if (AWidget)
	{
		disconnect(AWidget->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
		disconnect(AWidget->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
		disconnect(AWidget->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
		disconnect(AWidget->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
		if (AWidget->tabPageNotifier())
		{
			disconnect(AWidget->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),this,SLOT(onTabPageNotifierNotifyInserted(int)));
			disconnect(AWidget->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),this,SLOT(onTabPageNotifierNotifyRemoved(int)));
		}
		disconnect(AWidget->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
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

bool MetaTabWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::Paint)
	{
		if (AObject == ui.tlbToolBar)
		{
			QToolButton *button = FPageButtons.value(currentPage());
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

		QToolButton *button = qobject_cast<QToolButton *>(AObject);
		if (button)
		{
			button->removeEventFilter(this);
			QApplication::sendEvent(button, AEvent);
			button->installEventFilter(this);

			QIcon notifyBalloon = button->property("notifyBalloon").value<QIcon>();
			QPainter p(button);
			if (!notifyBalloon.isNull())
			{
				QSize notifySize = notifyBalloon.availableSizes().at(0);
				QRect notifyRect = QRect(button->width() - notifySize.width(), 0, notifySize.width(), notifySize.height());
				p.drawPixmap(notifyRect, notifyBalloon.pixmap(notifySize));
			}

			QImage statusIcon = button->property("statusIcon").value<QImage>();
			if (!statusIcon.isNull())
			{
				QSize iconSize = statusIcon.size();
				QRect iconRect = QRect(button->width() - iconSize.width(), button->height() - iconSize.height(), iconSize.width(), iconSize.height());
				p.drawImage(iconRect, statusIcon);
			}

			p.end();
			return true;
		}
	}
	return QMainWindow::eventFilter(AObject, AEvent);
}

void MetaTabWindow::showEvent(QShowEvent *AEvent)
{
	if (!FShownDetached)
		loadWindowGeometry();
	FShownDetached = isWindow();
	checkCurrentPage();
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
	QString pageId = FPageActions.key(action);
	if (!pageId.isEmpty())
	{
		Menu *menu = new Menu(this);

		createItemContextMenu(pageItem(pageId),menu);
		emit pageContextMenuRequested(pageId,menu);

		if (menu && !menu->isEmpty())
			menu->exec(AEvent->globalPos());

		AEvent->accept();
		delete menu;
	}
}

void MetaTabWindow::onTabPageShow()
{
	ITabPage *widget = qobject_cast<ITabPage *>(sender());
	setCurrentPage(widgetPage(widget));
	showTabPage();
}

void MetaTabWindow::onTabPageClose()
{
	closeTabPage();
}

void MetaTabWindow::onTabPageChanged()
{
	ITabPage *widget = qobject_cast<ITabPage *>(sender());
	if (pageWidget(currentPage()) == widget)
		updateWindow();
}

void MetaTabWindow::onTabPageDestroyed()
{
	ITabPage *widget = qobject_cast<ITabPage *>(sender());
	setPageWidget(widgetPage(widget),NULL);
}

void MetaTabWindow::onTabPageNotifierChanged()
{
	ITabPage *widget = qobject_cast<ITabPage *>(sender());
	if (widget && widget->tabPageNotifier())
	{
		connect(widget->tabPageNotifier()->instance(),SIGNAL(notifyInserted(int)),SLOT(onTabPageNotifierNotifyInserted(int)));
		connect(widget->tabPageNotifier()->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTabPageNotifierNotifyRemoved(int)));
	}
	updatePageButtonNotify(widgetPage(widget));
}

void MetaTabWindow::onTabPageNotifierNotifyInserted(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	QString pageId = notifier!=NULL ? widgetPage(notifier->tabPage()) : QString::null;
	if (FTabPageNotifier && !pageId.isEmpty() && (!isActive() || currentPage()==pageId))
	{
		ITabPageNotify notify = notifier->notifyById(ANotifyId);
		int notifyId = FTabPageNotifier->insertNotify(notify);
		FTabPageNotifies.insert(ANotifyId,notifyId);
	}
	updatePageButtonNotify(pageId);
}

void MetaTabWindow::onTabPageNotifierNotifyRemoved(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	QString pageId = notifier!=NULL ? widgetPage(notifier->tabPage()) : QString::null;
	if (FTabPageNotifier && FTabPageNotifies.contains(ANotifyId))
	{
		int notifyId = FTabPageNotifies.take(ANotifyId);
		FTabPageNotifier->removeNotify(notifyId);
	}
	updatePageButtonNotify(pageId);
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
		QString message = tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(itemJid.bare());
		if (QMessageBox::question(NULL,tr("Remove contact"),message,QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			FMetaRoster->deleteContactItem(FMetaId,action->data(ADR_ITEM_JID).toString());
	}
}

void MetaTabWindow::onCurrentWidgetChanged(int AIndex)
{
	QString pageId = widgetPage(qobject_cast<ITabPage *>(ui.stwWidgets->widget(AIndex)));
	if (FPageActions.contains(pageId))
	{
		QToolButton *button = FPageButtons.value(pageId);
		button->setChecked(true);
		setButtonAction(button,FPageActions.value(pageId));
		updatePageButton(pageId);
		updateWindow();
		FLastItemJid = FItemPages.values().contains(pageId) ? pageItem(pageId) : FLastItemJid;
		emit currentPageChanged(pageId);
	}
	QTimer::singleShot(0,ui.tlbToolBar,SLOT(repaint()));
}

void MetaTabWindow::onMetaPresenceChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		foreach(Jid itemJid, FItemPages.keys())
			updateItemButtonStatus(itemJid);
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
			updateItemPages(AContact.items);
			updateWindow();
		}
		else
		{
			closeTabPage();
			deleteLater();
		}
	}
}

void MetaTabWindow::onPageButtonClicked(bool)
{
	Action *action = FButtonAction.value(qobject_cast<QToolButton *>(sender()));
	if (action)
		action->trigger();
}

void MetaTabWindow::onPageActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		setCurrentPage(FPageActions.key(action));
}
