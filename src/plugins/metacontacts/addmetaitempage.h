#ifndef ADDMETAITEMPAGE_H
#define ADDMETAITEMPAGE_H

#include <QWidget>
#include <interfaces/imessagewidgets.h>
#include "ui_addmetaitempage.h"

class AddMetaItemPage : 
	public QWidget,
	public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	AddMetaItemPage(QWidget *AParent = NULL);
	~AddMetaItemPage();
	//ITabPage
	virtual QWidget *instance() { return this; }
	virtual void showTabPage();
	virtual void closeTabPage();
	virtual bool isActive() const;
	virtual QString tabPageId() const;
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
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
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
private:
	Ui::AddMetaItemPage ui;
private:
};

#endif // ADDMETAITEMPAGE_H
