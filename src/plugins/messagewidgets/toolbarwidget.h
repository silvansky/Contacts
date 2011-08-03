#ifndef TOOLBARWIDGET_H
#define TOOLBARWIDGET_H

#include <QToolBar>
#include <interfaces/imessagewidgets.h>

class ToolBarWidget :
	public QToolBar,
	public IToolBarWidget
{
	Q_OBJECT
	Q_INTERFACES(IToolBarWidget)
public:
	ToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	~ToolBarWidget();
	virtual QToolBar *instance() { return this; }
	virtual ToolBarChanger *toolBarChanger() const;
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
	ToolBarChanger *FToolBarChanger;
};

#endif // TOOLBARWIDGET_H
