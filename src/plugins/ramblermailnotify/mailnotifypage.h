#ifndef MAILNOTIFYPAGE_H
#define MAILNOTIFYPAGE_H

#include <QWidget>
#include <definitions/textflags.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/namespaces.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_mailnotifypage.h"

class MailNotifyPage : 
	public QWidget,
	public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	MailNotifyPage(IMessageWidgets *AMessageWidgets, IRosterIndex *AMailIndex, const Jid &AServiceJid, QWidget *AParent = NULL);
	~MailNotifyPage();
	virtual QWidget *instance() { return this; }
	//ITabPage
	virtual QString tabPageId() const;
	virtual bool isActiveTabPage() const;
	virtual void assignTabPage();
	virtual void showTabPage();
	virtual void showMinimizedTabPage();
	virtual void closeTabPage();
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
	//MailNotifyPage
	virtual Jid streamJid() const;
	virtual Jid serviceJid() const;
	virtual int newMailsCount() const;
	virtual void appendNewMail(const Stanza &AStanza);
	virtual void clearNewMails();
signals:
	//ITabPage
	void tabPageAssign();
	void tabPageShow();
	void tabPageShowMinimized();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
	//MailNotifyPage
	void showCustomMailPage();
	void showChatWindow(const Jid &AContactJid);
protected:
	void clearBoldFont();
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onNewMailButtonClicked();
	void onGoToEmailButtonClicked();
	void onTableCellDoubleClicked(int ARow, int AColumn);
	void onMailIndexDataChanged(IRosterIndex *AIndex, int ARole = 0);
private:
	Ui::MailNotifyPageClass ui;
private:
	IMessageWidgets *FMessageWidgets;
	ITabPageNotifier *FTabPageNotifier;
private:
	Jid FServiceJid;
	IRosterIndex *FMailIndex;
};

#endif // MAILNOTIFYPAGE_H
