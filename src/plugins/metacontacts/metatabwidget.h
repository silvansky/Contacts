#ifndef METATABWIDGET_H
#define METATABWIDGET_H

#include <QWidget>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <utils/widgetmanager.h>
#include "ui_metatabwidget.h"

class MetaTabWidget :
		public QWidget,
		public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	MetaTabWidget(IMessageWidgets *AMessageWidgets, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const Jid &AMetaId, QWidget *AParent = NULL);
	~MetaTabWidget();
	virtual QWidget *instance() { return this; }
	//ITabPage
	virtual void showTabPage();
	virtual void closeTabPage();
	virtual QString tabPageId() const;
	virtual bool isActive() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
	//IMetaTabWidget
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
private:
	Ui::MetaTabWidgetClass ui;
private:
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
	IMessageWidgets *FMessageWidgets;
	ITabPageNotifier *FTabPageNotifier;
private:
	Jid FMetaId;
};

#endif // METATABWIDGET_H
