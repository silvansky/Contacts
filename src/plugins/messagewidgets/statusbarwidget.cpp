#include "statusbarwidget.h"

StatusBarWidget::StatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	FInfoWidget = AInfo;
	FViewWidget = AView;
	FEditWidget = AEdit;
	FReceiversWidget = AReceivers;
	FStatusBarChanger = new StatusBarChanger(this);
}

StatusBarWidget::~StatusBarWidget()
{

}

StatusBarChanger *StatusBarWidget::statusBarChanger() const
{
	return FStatusBarChanger;
}

IInfoWidget *StatusBarWidget::infoWidget() const
{
	return FInfoWidget;
}

IViewWidget *StatusBarWidget::viewWidget() const
{
	return FViewWidget;
}

IEditWidget *StatusBarWidget::editWidget() const
{
	return FEditWidget;
}

IReceiversWidget *StatusBarWidget::receiversWidget() const
{
	return FReceiversWidget;
}
