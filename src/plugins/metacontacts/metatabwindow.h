#ifndef METATABWINDOW_H
#define METATABWINDOW_H

#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/irosterchanger.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/toolbarchanger.h>
#include "addmetaitempage.h"
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
	virtual ToolBarChanger *toolBarChanger() const;
	virtual void insertTopWidget(int AOrder, QWidget *AWidget);
	virtual void removeTopWidget(QWidget *AWidget);
	//Common pages
	virtual void createFirstPage();
	virtual QList<QString> pages() const;
	virtual QString currentPage() const;
	virtual void setCurrentPage(const QString &APageId);
	virtual QString insertPage(int AOrder, bool ACombine = false);
	virtual QIcon pageIcon(const QString &APageId) const;
	virtual void setPageIcon(const QString &APageId, const QIcon &AIcon);
	virtual void setPageIcon(const QString &APageId, const QString &AMetaIcon);
	virtual QString pageName(const QString &APageId) const;
	virtual void setPageName(const QString &APageId, const QString &AName);
	virtual QString widgetPage(ITabPage *APage) const;
	virtual ITabPage *pageWidget(const QString &APageId) const;
	virtual void setPageWidget(const QString &APageId, ITabPage *AWidget);
	virtual void removePage(const QString &APageId);
	//Item pages
	virtual bool isContactPage() const;
	virtual Jid currentItem() const;
	virtual void setCurrentItem(const Jid &AItemJid);
	virtual Jid pageItem(const QString &APageId) const;
	virtual QString itemPage(const Jid &AItemJid) const;
	virtual ITabPage *itemWidget(const Jid &AItemJid) const;
	virtual void setItemWidget(const Jid &AItemJid, ITabPage *AWidget);
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
	void currentPageChanged(const QString &APageId);
	void pageInserted(const QString &APageId, int AOrder, bool ACombined);
	void pageChanged(const QString &APageId);
	void pageWidgetRequested(const QString &APageId);
	void pageContextMenuRequested(const QString &APageId, Menu *AMenu);
	void pageRemoved(const QString &APageId);
	void topWidgetInserted(int AOrder, QWidget *AWidget);
	void topWidgetRemoved(QWidget* AWidget);
protected:
	void initialize(IPluginManager *APluginManager);
	void updateWindow();
	void updatePageButton(const QString &APageId);
	void updatePageButtonNotify(const QString &APageId);
	void setButtonAction(QToolButton *AButton, Action *AAction);
	int pageNotifyCount(const QString &APageId, bool ACombined) const;
	QIcon createNotifyBalloon(int ACount) const;
private:
	Jid lastItemJid() const;
	void updateItemPages(const QSet<Jid> &AItems);
	void updateItemButtonStatus(const Jid &AItemJid);
	void createItemContextMenu(const Jid &AItemJid, Menu *AMenu) const;
protected:
	void createPersistantList();
	void updatePersistantPages();
	void insertPersistantWidget(const QString &APageId);
protected:
	void connectPageWidget(ITabPage *AWidget);
	void disconnectPageWidget(ITabPage *AWidget);
	void removeTabPageNotifies();
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	bool event(QEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void closeEvent(QCloseEvent *AEvent);
	void contextMenuEvent(QContextMenuEvent *AEvent);
protected slots:
	void onTabPageShow();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabPageNotifierChanged();
	void onTabPageNotifierNotifyInserted(int ANotifyId);
	void onTabPageNotifierNotifyRemoved(int ANotifyId);
protected slots:
	void onDetachItemByAction(bool);
	void onDeleteItemByAction(bool);
	void onDeleteItemConfirmed();
protected slots:
	void onCurrentWidgetChanged(int AIndex);
	void onMetaPresenceChanged(const QString &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
protected slots:
	void onPageButtonClicked(bool);
	void onPageActionTriggered(bool);
private:
	Ui::MetaTabWindowClass ui;
private:
	IPluginManager *FPluginManager;
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
	QMap<QString, Action *> FPageActions;
	QMultiMap<int, QString> FCombinedPages;
	QMap<QString, ITabPage *> FPageWidgets;
	QMap<QString, QToolButton *> FPageButtons;
	QMap<QToolButton *, Action *> FButtonAction;
private:
	Jid FLastItemJid;
	QMap<Jid, QString> FItemPages;
	QMap<int, int> FItemTypeCount;
private:
	static QList<int> FPersistantList;
	QMap<int, QString> FPersistantPages;
};

#endif // METATABWINDOW_H
