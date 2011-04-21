#ifndef MAILNOTIFYPAGE_H
#define MAILNOTIFYPAGE_H

#include <QWidget>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
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
	MailNotifyPage(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
	~MailNotifyPage();
	virtual QWidget *instance() { return this; }
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
	//MailNotifyPage
	virtual Jid streamJid() const;
	virtual Jid serviceJid() const;
signals:
	void tabPageShow();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onNewMailButtonClicked();
	void onIncomingButtonClicked();
private:
	Ui::MailNotifyPageClass ui;
private:
	IMessageWidgets *FMessageWidgets;
	ITabPageNotifier *FTabPageNotifier;
private:
	Jid FStreamJid;
	Jid FServiceJid;
	QString FTabPageToolTip;
};

#endif // MAILNOTIFYPAGE_H
