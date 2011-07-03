#ifndef ADDMETAITEMPAGE_H
#define ADDMETAITEMPAGE_H

#include <QWidget>
#include <definitions/textflags.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/resources.h>
#include <definitions/gateserviceidentifiers.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/imessageprocessor.h>
#include <utils/stylestorage.h>
#include "ui_addmetaitempage.h"

class AddMetaItemPage :
	public QWidget,
	public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	AddMetaItemPage(IPluginManager *APluginManager, IMetaTabWindow *AMetaTabWindow,	const IMetaItemDescriptor &ADescriptor, QWidget *AParent = NULL);
	~AddMetaItemPage();
	//ITabPage
	virtual QWidget *instance() { return this; }
	virtual void assignTabPage();
	virtual void showTabPage();
	virtual void showMinimizedTabPage();
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
protected:
	void initialize(IPluginManager *APluginManager);
	QString infoMessageForGate();
	void setErrorMessage(const QString &AMessage);
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onAppendContactButtonClicked();
	void onItemWidgetErrorMessageClicked();
	void onItemWidgetContactJidChanged();
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
private:
	Ui::AddMetaItemPageClass ui;
private:
	IMetaTabWindow *FMetaTabWindow;
	IRosterChanger *FRosterChanger;
	IAddMetaItemWidget *FAddWidget;
	IMessageProcessor *FMessageProcessor;
private:
	QString FCreateRequestId;
	QString FMergeRequestId;
private:
	IMetaItemDescriptor FDescriptor;
};

#endif // ADDMETAITEMPAGE_H
