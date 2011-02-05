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
	Q_OBJECT;
	Q_INTERFACES(IMetaTabWindow ITabPage);
public:
	MetaTabWindow(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent = NULL);
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
	virtual Jid metaId() const;
	virtual IMetaRoster *metaRoster() const;
	virtual ToolBarChanger *toolBarChanger() const;
	virtual ITabPage *itemPage(const Jid &AItemJid) const;
	virtual void setItemPage(const Jid &AItemJid, ITabPage *APage);
	virtual Jid currentItem() const;
	virtual void setCurrentItem(const Jid &AItemJid);
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
	void intemContextMenuRequested(const Jid &AItemJid, Menu *AMenu);
protected:
	void initialize(IPluginManager *APluginManager);
	Jid firstItemJid() const;
	void updateWindow();
	void updateItemButton(const Jid &AItemJid);
	void updateItemButtons(const QSet<Jid> &AItems);
	QIcon insertNotifyBalloon(const QIcon &AIcon, int ACount) const;
	void removeTabPageNotifies();
	void saveWindowGeometry();
	void loadWindowGeometry();
	void createItemContextMenu(const Jid &AItemJid, Menu *AMenu) const;
protected:
	virtual bool event(QEvent *AEvent);
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
	void onItemButtonActionTriggered(bool);
	void onCurrentWidgetChanged(int AIndex);
	void onMetaPresenceChanged(const Jid &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
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
	Jid FMetaId;
	bool FShownDetached;
	QString FTabPageToolTip;
	ToolBarChanger *FToolBarChanger;
	QMap<int,int> FTabPageNotifies;
	QMap<Jid, ITabPage *> FItemTabPages;
	QMap<Jid, QToolButton *> FItemButtons;
};

#endif // METATABWINDOW_H
