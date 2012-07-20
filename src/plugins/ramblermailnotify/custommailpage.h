#ifndef CUSTOMMAILPAGE_H
#define CUSTOMMAILPAGE_H

#include <QWidget>
#include <definitions/textflags.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/igateways.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_custommailpage.h"

class CustomMailPage : 
	public QWidget,
	public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	CustomMailPage(IGateways *AGateways, IMessageWidgets *AMessageWidgets, IRosterIndex *AMailIndex, const Jid &AServiceJid, QWidget *AParent = NULL);
	~CustomMailPage();
	//ITabPage
	virtual QWidget *instance() { return this; }
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
	//CustomMailPage
	virtual Jid streamJid() const;
	virtual Jid serviceJid() const;
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
	//CustomMailPage
	void showChatWindow(const Jid &AContactJid);
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onContinueButtonClicked();
	void onContactMailChanged(const QString &AText);
	void onMailIndexDataChanged(IRosterIndex *AIndex, int ARole = 0);
	void onUserJidReceived(const QString &AId, const Jid &AUserJid);
	void onErrorReceived(const QString &AId, const QString &AError);
private:
	Ui::CustomMailPageClass ui;
private:
	IGateways *FGateways;
	IMessageWidgets *FMessageWidgets;
private:
	QString FRequestId;
private:
	Jid FServiceJid;
	IRosterIndex *FMailIndex;
	IGateServiceDescriptor FDescriptor;
};

#endif // CUSTOMMAILPAGE_H
