#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/messagedataroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istatuschanger.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_chatwindow.h"

class ChatWindow :
			public QMainWindow,
			public IChatWindow
{
	Q_OBJECT;
	Q_INTERFACES(IChatWindow ITabPage);
public:
	ChatWindow(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	virtual ~ChatWindow();
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
	//IChatWindow
	virtual const Jid &streamJid() const { return FStreamJid; }
	virtual const Jid &contactJid() const { return FContactJid; }
	virtual void setContactJid(const Jid &AContactJid);
	virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
	virtual IViewWidget *viewWidget() const { return FViewWidget; }
	virtual IChatNoticeWidget *noticeWidget() const { return FNoticeWidget; }
	virtual IEditWidget *editWidget() const { return FEditWidget; }
	virtual IMenuBarWidget *menuBarWidget() const { return FMenuBarWidget; }
	virtual IToolBarWidget *toolBarWidget() const { return FToolBarWidget; }
	virtual IStatusBarWidget *statusBarWidget() const { return FStatusBarWidget; }
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle, const QString &AToolTip);
	// Расширен функционал IChatWindow. Позволяет добавлять виджеты в окно чата
	virtual void insertTopWidget(int AOrder, QWidget *AWidget);
	virtual void removeTopWidget(QWidget *AWidget);
	virtual void insertBottomWidget(int AOrder, QWidget *AWidget);
	virtual void removeBottomWidget(QWidget *AWidget);
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
	//IChatWindow
	void messageReady();
	void streamJidChanged(const Jid &ABefour);
	void contactJidChanged(const Jid &ABefour);
	// Попов С.А.
	void topWidgetInserted(int AOrder, QWidget *AWidget);
	void topWidgetRemoved(QWidget *AWidget);
	void bottomWidgetInserted(int AOrder, QWidget *AWidget);
	void bottomWidgetRemoved(QWidget *AWidget);
protected:
	void initialize();
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onMessageReady();
	void onStreamJidChanged(const Jid &ABefour);
	void onOptionsChanged(const OptionsNode &ANode);
	void onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu);
	void onViewContextQuoteActionTriggered(bool);
	void onNoticeActivated(int ANoticeId);
private:
	Ui::ChatWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
	IStatusChanger *FStatusChanger;
private:
	IInfoWidget *FInfoWidget;
	IViewWidget *FViewWidget;
	IChatNoticeWidget *FNoticeWidget;
	IEditWidget *FEditWidget;
	IMenuBarWidget *FMenuBarWidget;
	IToolBarWidget *FToolBarWidget;
	IStatusBarWidget *FStatusBarWidget;
	ITabPageNotifier *FTabPageNotifier;
private:
	Jid FStreamJid;
	Jid FContactJid;
	bool FShownDetached;
	QString FTabPageToolTip;
};

#endif // CHATWINDOW_H
