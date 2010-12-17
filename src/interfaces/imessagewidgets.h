#ifndef IMESSAGEWIDGETS_H
#define IMESSAGEWIDGETS_H

#include <QUuid>
#include <QMultiMap>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestyles.h>
#include <utils/jid.h>
#include <utils/menu.h>
#include <utils/action.h>
#include <utils/message.h>
#include <utils/menubarchanger.h>
#include <utils/toolbarchanger.h>
#include <utils/statusbarchanger.h>

#define MESSAGEWIDGETS_UUID "{fe5f3695-2343-4642-92f1-ab745fb26e13}"

class IInfoWidget
{
public:
	enum InfoField {
		AccountName         =1,
		ContactName         =2,
		ContactShow         =4,
		ContactStatus       =8,
		ContactAvatar       =16
	};
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual void autoUpdateFields() =0;
	virtual void autoUpdateField(InfoField AField) =0;
	virtual QVariant field(InfoField AField) const =0;
	virtual void setField(InfoField AField, const QVariant &AValue) =0;
	virtual int autoUpdatedFields() const =0;
	virtual bool isFiledAutoUpdated(IInfoWidget::InfoField AField) const =0;
	virtual void setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto) =0;
	virtual int visibleFields() const =0;
	virtual bool isFieldVisible(IInfoWidget::InfoField AField) const =0;
	virtual void setFieldVisible(IInfoWidget::InfoField AField, bool AVisible) =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue) =0;
};

class IViewWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QWidget *styleWidget() const =0;
	virtual IMessageStyle *messageStyle() const =0;
	virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions) =0;
	virtual QUuid changeContentHtml(const QString &AHtml, const IMessageContentOptions &AOptions) =0;
	virtual QUuid changeContentText(const QString &AText, const IMessageContentOptions &AOptions) =0;
	virtual QUuid changeContentMessage(const Message &AMessage, const IMessageContentOptions &AOptions) =0;
	virtual void contextMenuForView(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu) =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void messageStyleChanged(IMessageStyle *ABefour, const IMessageStyleOptions &AOptions) =0;
	virtual void contentChanged(const QUuid &AContentId, const QString &AHtml, const IMessageContentOptions &AOptions) =0;
	virtual void viewContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu) =0;
	virtual void urlClicked(const QUrl &AUrl) const =0;
};

struct IChatNotice
{
	IChatNotice() {
		priority = -1;
		timeout = 0;
	}
	int priority;
	int timeout;
	QIcon icon;
	QString iconKey;
	QString iconStorage;
	QString message;
	QList<Action *> actions;
};

class IChatNoticeWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual int activeNotice() const =0;
	virtual QList<int> noticeQueue() const =0;
	virtual IChatNotice noticeById(int ANoticeId) const =0;
	virtual int insertNotice(const IChatNotice &ANotice) =0;
	virtual void removeNotice(int ANoticeId) =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void noticeInserted(int ANoticeId) =0;
	virtual void noticeActivated(int ANoticeId) =0;
	virtual void noticeRemoved(int ANoticeId) =0;
};

class IEditWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QTextEdit *textEdit() const =0;
	virtual QTextDocument *document() const =0;
	virtual void sendMessage() =0;
	virtual void clearEditor() =0;
	virtual bool autoResize() const =0;
	virtual void setAutoResize(bool AResize) =0;
	virtual int minimumLines() const =0;
	virtual void setMinimumLines(int ALines) =0;
	virtual QKeySequence sendKey() const =0;
	virtual void setSendKey(const QKeySequence &AKey) =0;
	virtual bool sendButtonVisible() const =0;
	virtual void setSendButtonVisible(bool AVisible) =0;
	virtual bool textFormatEnabled() const =0;
	virtual void setTextFormatEnabled(bool AEnabled) =0;
protected:
	virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook) =0;
	virtual void messageAboutToBeSend() =0;
	virtual void messageReady() =0;
	virtual void editorCleared() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
	virtual void autoResizeChanged(bool AResize) =0;
	virtual void minimumLinesChanged(int ALines) =0;
	virtual void sendKeyChanged(const QKeySequence &AKey) =0;
};

class IReceiversWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual const Jid &streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual QList<Jid> receivers() const =0;
	virtual QString receiverName(const Jid &AReceiver) const =0;
	virtual void addReceiversGroup(const QString &AGroup) =0;
	virtual void removeReceiversGroup(const QString &AGroup) =0;
	virtual void addReceiver(const Jid &AReceiver) =0;
	virtual void removeReceiver(const Jid &AReceiver) =0;
	virtual void clear() =0;
protected:
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void receiverAdded(const Jid &AReceiver) =0;
	virtual void receiverRemoved(const Jid &AReceiver) =0;
};

class IMenuBarWidget
{
public:
	virtual QMenuBar *instance() = 0;
	virtual MenuBarChanger *menuBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

class IToolBarWidget
{
public:
	virtual QToolBar *instance() = 0;
	virtual ToolBarChanger *toolBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

class IStatusBarWidget
{
public:
	virtual QStatusBar *instance() = 0;
	virtual StatusBarChanger *statusBarChanger() const =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
};

struct ITabPageNotify
{
	ITabPageNotify() {
		priority = -1;
		blink = true;
	}
	int priority;
	bool blink;
	QIcon icon;
	QString iconKey;
	QString iconStorage;
	QString toolTip;
	QString styleKey;
};

class ITabPage;
class ITabPageNotifier
{
public:
	virtual QObject *instance() =0;
	virtual ITabPage *tabPage() const =0;
	virtual int activeNotify() const =0;
	virtual QList<int> notifies() const =0;
	virtual ITabPageNotify notifyById(int ANotifyId) const =0;
	virtual int insertNotify(const ITabPageNotify &ANotify) =0;
	virtual void removeNotify(int ANotifyId) =0;
protected:
	virtual void notifyInserted(int ANotifyId) =0;
	virtual void notifyRemoved(int ANotifyId) =0;
	virtual void activeNotifyChanged(int ANotifyId) =0;
};

class ITabPage
{
public:
	virtual QWidget *instance() =0;
	virtual void showTabPage() =0;
	virtual void closeTabPage() =0;
	virtual QString tabPageId() const =0;
	virtual bool isActive() const =0;
	virtual ITabPageNotifier *tabPageNotifier() const =0;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier) =0;
protected:
	virtual void tabPageShow() =0;
	virtual void tabPageClose() =0;
	virtual void tabPageClosed() =0;
	virtual void tabPageChanged() =0;
	virtual void tabPageActivated() =0;
	virtual void tabPageDeactivated() =0;
	virtual void tabPageDestroyed() =0;
	virtual void tabPageNotifierChanged() =0;
};

class ITabWindow
{
public:
	virtual QMainWindow *instance() = 0;
	virtual void showWindow() =0;
	virtual QUuid windowId() const =0;
	virtual QString windowName() const =0;
	virtual Menu *windowMenu() const =0;
	virtual void addTabPage(ITabPage *APage) =0;
	virtual bool hasTabPage(ITabPage *APage) const=0;
	virtual ITabPage *currentTabPage() const =0;
	virtual void setCurrentTabPage(ITabPage *APage) =0;
	virtual void detachTabPage(ITabPage *APage) =0;
	virtual void removeTabPage(ITabPage *APage) =0;
protected:
	virtual void currentTabPageChanged(ITabPage *APage) =0;
	virtual void tabPageAdded(ITabPage *APage) =0;
	virtual void tabPageRemoved(ITabPage *APage) =0;
	virtual void tabPageDetached(ITabPage *APage) =0;
	virtual void windowChanged() =0;
	virtual void windowDestroyed() =0;
};

class IChatWindow :
		public ITabPage
{
public:
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IChatNoticeWidget *noticeWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IMenuBarWidget *menuBarWidget() const =0;
	virtual IToolBarWidget *toolBarWidget() const =0;
	virtual IStatusBarWidget *statusBarWidget() const =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle, const QString &AToolTip) =0;
	// Расширен функционал IChatWindow. Позволяет добавлять виджеты в окно чата
	virtual void insertTopWidget(int AOrder, QWidget *AWidget) =0;
	virtual void removeTopWidget(QWidget *AWidget) =0;
	virtual void insertBottomWidget(int AOrder, QWidget *AWidget) =0;
	virtual void removeBottomWidget(QWidget *AWidget) =0;
protected:
	virtual void messageReady() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
};

class IMessageWindow :
		public ITabPage
{
public:
	enum Mode
	{
		ReadMode    =1,
		WriteMode   =2
	};

public:
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual void addTabWidget(QWidget *AWidget) =0;
	virtual void setCurrentTabWidget(QWidget *AWidget) =0;
	virtual void removeTabWidget(QWidget *AWidget) =0;
	virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
	virtual IToolBarWidget *viewToolBarWidget() const =0;
	virtual IToolBarWidget *editToolBarWidget() const =0;
	virtual Mode mode() const =0;
	virtual void setMode(Mode AMode) =0;
	virtual QString subject() const =0;
	virtual void setSubject(const QString &ASubject) =0;
	virtual QString threadId() const =0;
	virtual void setThreadId(const QString &AThreadId) =0;
	virtual int nextCount() const =0;
	virtual void setNextCount(int ACount) =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle) =0;
protected:
	virtual void showNextMessage() =0;
	virtual void replyMessage() =0;
	virtual void forwardMessage() =0;
	virtual void showChatWindow() =0;
	virtual void messageReady() =0;
	virtual void streamJidChanged(const Jid &ABefour) =0;
	virtual void contactJidChanged(const Jid &ABefour) =0;
};

class IMassSendDialog :
		public ITabPage
{
public:
	virtual QDialog * instance() =0;
	virtual const Jid &streamJid() const =0;
	//virtual IInfoWidget *infoWidget() const =0;
	virtual IViewWidget *viewWidget() const =0;
	virtual IEditWidget *editWidget() const =0;
	virtual IReceiversWidget *receiversWidget() const =0;
protected:
	virtual void messageReady() =0;
};

class IViewDropHandler
{
public:
	virtual bool viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent) =0;
	virtual bool viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent) =0;
	virtual void viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent) =0;
	virtual bool viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu) =0;
};

class IViewUrlHandler
{
public:
	virtual bool viewUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder) =0;
};

class ITabPageHandler
{
public:
	virtual QObject *instance() = 0;
	virtual bool tabPageAvail(const QString &ATabPageId) const =0;
	virtual ITabPage *tabPageFind(const QString &ATabPageId) const =0;
	virtual ITabPage *tabPageCreate(const QString &ATabPageId) =0;
	virtual Action *tabPageAction(const QString &ATabPageId, QObject *AParent) =0;
protected:
	virtual void tabPageCreated(ITabPage *ATabPage) =0;
	virtual void tabPageDestroyed(ITabPage *ATabPage) =0;
};

class IMessageWidgets
{
public:
	virtual QObject *instance() = 0;
	virtual IPluginManager *pluginManager() const =0;
	virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IChatNoticeWidget *newNoticeWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid) =0;
	virtual IMenuBarWidget *newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual IStatusBarWidget *newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
	virtual ITabPageNotifier *newTabPageNotifier(ITabPage *ATabPage) = 0;
	virtual QList<IMessageWindow *> messageWindows() const =0;
	virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode) =0;
	virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<IMassSendDialog*> massSendDialogs() const =0;
	virtual IMassSendDialog * newMassSendDialog(const Jid & AStreamJid) =0;
	virtual IMassSendDialog * findMassSendDialog(const Jid & AStreamJid) =0;
	virtual QList<IChatWindow *> chatWindows() const =0;
	virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<QUuid> tabWindowList() const =0;
	virtual QUuid appendTabWindow(const QString &AName) =0;
	virtual void deleteTabWindow(const QUuid &AWindowId) =0;
	virtual QString tabWindowName(const QUuid &AWindowId) const =0;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName) =0;
	virtual QList<ITabWindow *> tabWindows() const =0;
	virtual ITabWindow *createTabWindow(const QUuid &AWindowId) =0;
	virtual ITabWindow *findTabWindow(const QUuid &AWindowId) const =0;
	virtual ITabWindow *assignTabWindowPage(ITabPage *APage) =0;
	virtual QList<IViewDropHandler *> viewDropHandlers() const =0;
	virtual void insertViewDropHandler(IViewDropHandler *AHandler) =0;
	virtual void removeViewDropHandler(IViewDropHandler *AHandler) =0;
	virtual QMultiMap<int, IViewUrlHandler *> viewUrlHandlers() const =0;
	virtual void insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual void removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual QList<ITabPageHandler *> tabPageHandlers() const =0;
	virtual void insertTabPageHandler(ITabPageHandler *AHandler) =0;
	virtual void removeTabPageHandler(ITabPageHandler *AHandler) =0;
protected:
	virtual void infoWidgetCreated(IInfoWidget *AInfoWidget) =0;
	virtual void viewWidgetCreated(IViewWidget *AViewWidget) =0;
	virtual void noticeWidgetCreated(IChatNoticeWidget *ANoticeWidget) =0;
	virtual void editWidgetCreated(IEditWidget *AEditWidget) =0;
	virtual void receiversWidgetCreated(IReceiversWidget *AReceiversWidget) =0;
	virtual void menuBarWidgetCreated(IMenuBarWidget *AMenuBarWidget) =0;
	virtual void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget) =0;
	virtual void statusBarWidgetCreated(IStatusBarWidget *AStatusBarWidget) =0;
	virtual void tabPageNotifierCreated(ITabPageNotifier *ANotifier) =0;
	virtual void messageWindowCreated(IMessageWindow *AWindow) =0;
	virtual void messageWindowDestroyed(IMessageWindow *AWindow) =0;
	virtual void massSendDialogCreated(IMassSendDialog*) =0;
	virtual void massSendDialogDestroyed(IMassSendDialog*) =0;
	virtual void chatWindowCreated(IChatWindow *AWindow) =0;
	virtual void chatWindowDestroyed(IChatWindow *AWindow) =0;
	virtual void tabWindowAppended(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowDeleted(const QUuid &AWindowId) =0;
	virtual void tabWindowCreated(ITabWindow *AWindow) =0;
	virtual void tabWindowDestroyed(ITabWindow *AWindow) =0;
	virtual void viewDropHandlerInserted(IViewDropHandler *AHandler) =0;
	virtual void viewDropHandlerRemoved(IViewDropHandler *AHandler) =0;
	virtual void viewUrlHandlerInserted(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual void viewUrlHandlerRemoved(IViewUrlHandler *AHandler, int AOrder) =0;
	virtual void tabPageHandlerInserted(ITabPageHandler *AHandler) =0;
	virtual void tabPageHandlerRemoved(ITabPageHandler *AHandler) =0;
};

Q_DECLARE_INTERFACE(IInfoWidget,"Virtus.Plugin.IInfoWidget/1.0")
Q_DECLARE_INTERFACE(IViewWidget,"Virtus.Plugin.IViewWidget/1.0")
Q_DECLARE_INTERFACE(IEditWidget,"Virtus.Plugin.IEditWidget/1.0")
Q_DECLARE_INTERFACE(IChatNoticeWidget,"Virtus.Plugin.IChatNoticeWidget/1.0")
Q_DECLARE_INTERFACE(IReceiversWidget,"Virtus.Plugin.IReceiversWidget/1.0")
Q_DECLARE_INTERFACE(IMenuBarWidget,"Virtus.Plugin.IMenuBarWidget/1.0")
Q_DECLARE_INTERFACE(IToolBarWidget,"Virtus.Plugin.IToolBarWidget/1.0")
Q_DECLARE_INTERFACE(IStatusBarWidget,"Virtus.Plugin.IStatusBarWidget/1.0")
Q_DECLARE_INTERFACE(ITabPage,"Virtus.Plugin.ITabPage/1.0")
Q_DECLARE_INTERFACE(ITabPageNotifier,"Virtus.Plugin.ITabPageNotifier/1.0")
Q_DECLARE_INTERFACE(ITabWindow,"Virtus.Plugin.ITabWindow/1.0")
Q_DECLARE_INTERFACE(IChatWindow,"Virtus.Plugin.IChatWindow/1.0")
Q_DECLARE_INTERFACE(IMessageWindow,"Virtus.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IMassSendDialog,"Virtus.Plugin.IMassSendDialog/1.0")
Q_DECLARE_INTERFACE(IViewDropHandler,"Virtus.Plugin.IViewDropHandler/1.0")
Q_DECLARE_INTERFACE(IViewUrlHandler,"Virtus.Plugin.IViewUrlHandler/1.0")
Q_DECLARE_INTERFACE(ITabPageHandler,"Virtus.Plugin.ITabPageHandler/1.0")
Q_DECLARE_INTERFACE(IMessageWidgets,"Virtus.Plugin.IMessageWidgets/1.0")

#endif
