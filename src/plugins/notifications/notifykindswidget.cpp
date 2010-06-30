#include "notifykindswidget.h"

NotifyKindsWidget::NotifyKindsWidget(INotifications *ANotifications, const QString &AId, const QString &ATitle, uchar AKindMask, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.lblTitle->setText(ATitle);

	FNotifications = ANotifications;
	FNotificatorId = AId;
	FNotificatorKindMask = AKindMask;

	ui.chbPopup->setEnabled(AKindMask & INotification::PopupWindow);
	ui.chbSound->setEnabled(AKindMask & INotification::PlaySound);

	connect(ui.chbPopup,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbSound,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.lblTest,SIGNAL(linkActivated(const QString &)),SLOT(onTestLinkActivated(const QString &)));

	reset();
}

NotifyKindsWidget::~NotifyKindsWidget()
{

}

void NotifyKindsWidget::apply()
{
	uchar kinds = FNotifications->notificatorKinds(FNotificatorId);
	if (ui.chbPopup->isChecked())
		kinds |= INotification::PopupWindow;
	else
		kinds &= ~INotification::PopupWindow;
	
	if (ui.chbSound->isChecked())
		kinds |= INotification::PlaySound;
	else
		kinds &= ~INotification::PlaySound;

	FNotifications->setNotificatorKinds(FNotificatorId,kinds);
	emit childApply();
}

void NotifyKindsWidget::reset()
{
	uchar kinds = FNotifications->notificatorKinds(FNotificatorId);
	ui.chbPopup->setChecked(kinds & INotification::PopupWindow);
	ui.chbSound->setChecked(kinds & INotification::PlaySound);
	emit childReset();
}

void NotifyKindsWidget::onTestLinkActivated(const QString &ALink)
{
	Q_UNUSED(ALink);
}
