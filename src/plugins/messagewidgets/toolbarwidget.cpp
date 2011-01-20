#include "toolbarwidget.h"

ToolBarWidget::ToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	FInfoWidget = AInfo;
	FViewWidget = AView;
	FEditWidget = AEdit;
	FReceiversWidget = AReceivers;
	FToolBarChanger = new ToolBarChanger(this);
	FToolBarChanger->setObjectName("toolBarChanger");
	setIconSize(QSize(16,16));
}

ToolBarWidget::~ToolBarWidget()
{

}
