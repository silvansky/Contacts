#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QStatusBar>
#include <interfaces/imessagewidgets.h>

class StatusBarWidget :
	public QStatusBar,
	public IStatusBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IStatusBarWidget);
public:
	StatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	~StatusBarWidget();
	virtual QStatusBar *instance() { return this; }
	virtual StatusBarChanger *statusBarChanger() const;
	virtual IInfoWidget *infoWidget() const;
	virtual IViewWidget *viewWidget() const;
	virtual IEditWidget *editWidget() const;
	virtual IReceiversWidget *receiversWidget() const;
private:
	IInfoWidget *FInfoWidget;
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IReceiversWidget *FReceiversWidget;
private:
	StatusBarChanger *FStatusBarChanger;
};

#endif // STATUSBARWIDGET_H
