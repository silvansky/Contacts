#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QDesktopServices>
#include <QObjectCleanupHandler>
#include <definitions/resources.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/viewurlhandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ivcard.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>
#include "infowidget.h"
#include "viewwidget.h"
#include "editwidget.h"
#include "noticewidget.h"
#include "receiverswidget.h"
#include "menubarwidget.h"
#include "toolbarwidget.h"
#include "statusbarwidget.h"
#include "tabpagenotifier.h"
#include "messagewindow.h"
#include "chatwindow.h"
#include "tabwindow.h"
#include "messengeroptions.h"
#include "masssenddialog.h"

class MessageWidgets :
	public QObject,
	public IPlugin,
	public IMessageWidgets,
	public IOptionsHolder,
	public IViewUrlHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageWidgets IOptionsHolder IViewUrlHandler)
public:
	MessageWidgets();
	~MessageWidgets();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MESSAGEWIDGETS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IViewUrlHandler
	virtual bool viewUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder);
	//IMessageWidgets
	virtual IPluginManager *pluginManager() const { return FPluginManager; }
	virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IChatNoticeWidget *newNoticeWidget(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid);
	virtual IMenuBarWidget *newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	virtual IStatusBarWidget *newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	virtual ITabPageNotifier *newTabPageNotifier(ITabPage *ATabPage);
	virtual QList<IMessageWindow *> messageWindows() const;
	virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
	virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<IChatWindow *> chatWindows() const;
	virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<IMassSendDialog*> massSendDialogs() const;
	virtual IMassSendDialog * newMassSendDialog(const Jid &AStreamJid);
	virtual IMassSendDialog * findMassSendDialog(const Jid &AStreamJid);
	virtual QList<QUuid> tabWindowList() const;
	virtual QUuid appendTabWindow(const QString &AName);
	virtual void deleteTabWindow(const QUuid &AWindowId);
	virtual QString tabWindowName(const QUuid &AWindowId) const;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName);
	virtual QList<ITabWindow *> tabWindows() const;
	virtual ITabWindow *newTabWindow(const QUuid &AWindowId);
	virtual ITabWindow *findTabWindow(const QUuid &AWindowId) const;
	virtual ITabWindow *assignTabWindowPage(ITabPage *APage);
	virtual QList<IViewDropHandler *> viewDropHandlers() const;
	virtual void insertViewDropHandler(IViewDropHandler *AHandler);
	virtual void removeViewDropHandler(IViewDropHandler *AHandler);
	virtual QMultiMap<int, IViewUrlHandler *> viewUrlHandlers() const;
	virtual void insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder);
	virtual void removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder);
	virtual QList<ITabPageHandler *> tabPageHandlers() const;
	virtual void insertTabPageHandler(ITabPageHandler *AHandler);
	virtual void removeTabPageHandler(ITabPageHandler *AHandler);
signals:
	void infoWidgetCreated(IInfoWidget *AInfoWidget);
	void viewWidgetCreated(IViewWidget *AViewWidget);
	void noticeWidgetCreated(IChatNoticeWidget *ANoticeWidget);
	void editWidgetCreated(IEditWidget *AEditWidget);
	void receiversWidgetCreated(IReceiversWidget *AReceiversWidget);
	void menuBarWidgetCreated(IMenuBarWidget *AMenuBarWidget);
	void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget);
	void statusBarWidgetCreated(IStatusBarWidget *AStatusBarWidget);
	void tabPageNotifierCreated(ITabPageNotifier *ANotifier);
	void messageWindowCreated(IMessageWindow *AWindow);
	void messageWindowDestroyed(IMessageWindow *AWindow);
	void massSendDialogCreated(IMassSendDialog*);
	void massSendDialogDestroyed(IMassSendDialog*);
	void chatWindowCreated(IChatWindow *AWindow);
	void chatWindowDestroyed(IChatWindow *AWindow);
	void tabWindowAppended(const QUuid &AWindowId, const QString &AName);
	void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
	void tabWindowDeleted(const QUuid &AWindowId);
	void tabWindowCreated(ITabWindow *AWindow);
	void tabWindowDestroyed(ITabWindow *AWindow);
	void viewDropHandlerInserted(IViewDropHandler *AHandler);
	void viewDropHandlerRemoved(IViewDropHandler *AHandler);
	void viewUrlHandlerInserted(IViewUrlHandler *AHandler, int AOrder);
	void viewUrlHandlerRemoved(IViewUrlHandler *AHandler, int AOrder);
	void tabPageHandlerInserted(ITabPageHandler *AHandler);
	void tabPageHandlerRemoved(ITabPageHandler *AHandler);
protected:
	void deleteWindows();
	void deleteStreamWindows(const Jid &AStreamJid);
	QList<Action *> createLastTabPagesActions(QObject *AParent) const;
protected slots:
	void onViewWidgetUrlClicked(const QUrl &AUrl);
	void onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu);
	void onViewContextCopyActionTriggered(bool);
	void onViewContextUrlActionTriggered(bool);
	void onViewContextSearchActionTriggered(bool);
	void onMessageWindowDestroyed();
	void onChatWindowDestroyed();
	void onTabPageAdded(ITabPage *APage);
	void onTabPageCreated(ITabPage *APage);
	void onTabPageActivated();
	void onTabWindowDestroyed();
	void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onTrayContextMenuAboutToShow();
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onOptionsOpened();
	void onOptionsClosed();
	void onMassSend();
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	ITrayManager *FTrayManager;
	IOptionsManager *FOptionsManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IAccountManager *FAccountManager;
	IVCardPlugin *FVCardPlugin;
private:
	QList<ITabWindow *> FTabWindows;
	QList<IChatWindow *> FChatWindows;
	QList<IMessageWindow *> FMessageWindows;
	QList<IMassSendDialog *> FMassSendDialogs;
	QObjectCleanupHandler FCleanupHandler;
private:
	QHash<QString, QUuid> FTabPageWindow;
	QList<ITabPageHandler *> FTabPageHandlers;
	QList<IViewDropHandler *> FViewDropHandlers;
	QMultiMap<int,IViewUrlHandler *> FViewUrlHandlers;
private:
	QList<QString> FLastPagesOrder;
	QMap<QString, QDateTime> FLastPagesActivity;
};

#endif // MESSAGEWIDGETS_H
