#ifndef ADDMETAITEMPAGE_H
#define ADDMETAITEMPAGE_H

#include <QWidget>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/resources.h>
#include <definitions/gateserviceidentifiers.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irosterchanger.h>
#include <utils/stylestorage.h>
#include "ui_addmetaitempage.h"

class AddMetaItemPage :
	public QWidget,
	public ITabPage
{
	Q_OBJECT;
	Q_INTERFACES(ITabPage);
public:
	AddMetaItemPage(IRosterChanger *ARosterChanger, IMetaTabWindow *AMetaTabWindow, IMetaRoster *AMetaRoster, const QString &AMetaId,
		const IMetaItemDescriptor &ADescriptor, QWidget *AParent = NULL);
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
	QString infoMessageForGate();
	void setErrorMessage(const QString &AMessage);
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onAppendContactButtonClicked();
	void onItemWidgetContactJidChanged(const Jid &AContactJid);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
private:
	Ui::AddMetaItemPageClass ui;
private:
	IMetaRoster *FMetaRoster;
	IMetaTabWindow *FMetaTabWindow;
	IRosterChanger *FRosterChanger;
	IAddMetaItemWidget *FAddWidget;
private:
	QString FCreateRequestId;
	QString FMergeRequestId;
private:
	QString FMetaId;
	IMetaItemDescriptor FDescriptor;
};

#endif // ADDMETAITEMPAGE_H
