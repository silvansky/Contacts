#ifndef METATABWINDOW_H
#define METATABWINDOW_H

#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/toolbargroups.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/toolbarchanger.h>
#include "ui_metatabwindow.h"

class MetaTabWindow :
		public QMainWindow,
		public IMetaTabWindow
{
	Q_OBJECT
	Q_INTERFACES(IMetaTabWindow ITabPage)
public:
	MetaTabWindow(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent = NULL);
	~MetaTabWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabPage
	virtual void showTabPage();
	virtual void closeTabPage();
	virtual bool isActive() const;
	virtual QString tabPageId() const;
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
	//IMetaTabWindow
	virtual QString metaId() const;
	virtual IMetaRoster *metaRoster() const;
	virtual ITabPage *itemPage(const Jid &AItemJid) const;
	virtual void setItemPage(const Jid &AItemJid, ITabPage *APage);
	virtual Jid currentItem() const;
	virtual void setCurrentItem(const Jid &AItemJid);
	
	virtual ITabPage *currentPage() const;
	virtual void setCurrentPage(ITabPage *APAge);
	virtual void insertPage(ITabPage *APage, int AOrder, bool ACombine = false);
	virtual void setPageIcon(ITabPage *APage, const QIcon &AIcon);
	virtual void setPageName(ITabPage *APage, const QString &AName);
	virtual void replacePage(ITabPage *AOldPage, ITabPage *ANewPage);
	virtual void removePage(ITabPage *APage);

	virtual ToolBarChanger *toolBarChanger() const;
	virtual void insertTopWidget(int AOrder, QWidget *AWidget);
	virtual void removeTopWidget(QWidget *AWidget);
signals:
	//ITabPage
	void tabPageShow();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
	//IMetaTabWindow
	void currentItemChanged(const Jid &AItemJid);
	void itemPageRequested(const Jid &AItemJid);
	void itemPageChanged(const Jid &AItemJid, ITabPage *APage);
	void itemContextMenuRequested(const Jid &AItemJid, Menu *AMenu);

	void currentPageChanged(ITabPage *APage);
	void pageInserted(ITabPage *APage, int AOrder, bool ACombined);
	void pageReplaced(ITabPage *AOldPage, ITabPage *ANewPage);
	void pageRemoved(ITabPage *APage);

	void topWidgetInserted(int AOrder, QWidget *AWidget);
	void topWidgetRemoved(QWidget* AWidget);
protected:
	void initialize(IPluginManager *APluginManager);
	Jid firstItemJid() const;
	void updateWindow();
	void updateItemAction(const Jid &AItemJid);
	void updateItemButton(const Jid &AItemJid);
	void updateItemButtons(const QSet<Jid> &AItems);
	void setButtonAction(QToolButton *AButton, Action *AAction);
	int itemNotifyCount(const Jid &AItemJid, bool ACombined) const;
	QIcon insertNotifyBalloon(const QIcon &AIcon, int ACount) const;
	void createItemContextMenu(const Jid &AItemJid, Menu *AMenu) const;
protected:
	void connectPage(ITabPage *APage);
	void disconnectPage(ITabPage *APage);
	void removeTabPageNotifies();
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	virtual bool event(QEvent *AEvent);
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual void contextMenuEvent(QContextMenuEvent *AEvent);
protected slots:
	void onTabPageShow();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabPageNotifierChanged();
	void onTabPageNotifierNotifyInserted(int ANotifyId);
	void onTabPageNotifierNotifyRemoved(int ANotifyId);
protected slots:
	void onEditItemByAction(bool);
	void onDetachItemByAction(bool);
	void onDeleteItemByAction(bool);
protected slots:
	void onItemButtonClicked(bool);
	void onItemActionTriggered(bool);
	void onCurrentWidgetChanged(int AIndex);
	void onMetaPresenceChanged(const QString &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
protected slots:
	void onPageButtonClicked(bool);
	void onPageActionTriggered(bool);
private:
	Ui::MetaTabWindowClass ui;
private:
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
	IMessageWidgets *FMessageWidgets;
	ITabPageNotifier *FTabPageNotifier;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
private:
	QString FMetaId;
	bool FShownDetached;
	QString FTabPageToolTip;
	ToolBarChanger *FToolBarChanger;
	QMap<int,int> FTabPageNotifies;
	QMap<Jid, ITabPage *> FItemTabPages;
	QMultiMap<QString, Jid> FCombinedItems;
	QMap<Jid, Action *> FItemActions;
	QMap<Jid, QToolButton *> FItemButtons;
private:
	QMap<ITabPage *, Action *> FPageActions;
	QMultiMap<int, ITabPage *> FCombinedPages;
	QMap<ITabPage *, QToolButton *> FPageButtons;
	QMap<QToolButton *, Action *> FButtonAction;
};

#endif // METATABWINDOW_H
