#include "notifykindswidget.h"

NotifyKindsWidget::NotifyKindsWidget(INotifications *ANotifications, const QString &ANotificatorId, const QString &ATitle, uchar AKindMask, uchar AKindDefs, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.lblTitle->setText(ATitle);
	ui.lblTest->setVisible(false);

	FNotifications = ANotifications;
	FNotificatorId = ANotificatorId;
	FNotificatorKindMask = AKindMask;
	FNotificatorKindDefs = AKindDefs;

	ui.chbPopup->setEnabled(AKindMask & INotification::PopupWindow);
	ui.chbSound->setEnabled(AKindMask & INotification::PlaySoundNotification);
	ui.lblTest->setEnabled(AKindMask & INotification::TestNotify);

	connect(ui.chbPopup,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbSound,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.lblTest,SIGNAL(linkActivated(const QString &)),SLOT(onTestLinkActivated(const QString &)));
	connect(ui.pbtTest, SIGNAL(clicked()), SLOT(onTestButtonClicked()));

	reset();
}

NotifyKindsWidget::~NotifyKindsWidget()
{

}

void NotifyKindsWidget::apply()
{
	FNotifications->setNotificatorKinds(FNotificatorId,changedKinds(FNotificatorKindDefs));
	emit childApply();
}

void NotifyKindsWidget::reset()
{
	uchar kinds = FNotifications->notificatorKinds(FNotificatorId);
	ui.chbPopup->setChecked(kinds & INotification::PopupWindow);
	ui.chbSound->setChecked(kinds & INotification::PlaySoundNotification);
	emit childReset();
}

uchar NotifyKindsWidget::changedKinds(uchar AActiveKinds) const
{
	uchar kinds = AActiveKinds;
	kinds &= ~INotification::TestNotify;

	if (ui.chbPopup->isChecked())
		kinds |= INotification::PopupWindow;
	else
		kinds &= ~INotification::PopupWindow;

	if (ui.chbSound->isChecked())
		kinds |= INotification::PlaySoundNotification;
	else
		kinds &= ~INotification::PlaySoundNotification;

	return kinds;
}

void NotifyKindsWidget::onTestLinkActivated(const QString &ALink)
{
	Q_UNUSED(ALink);
	emit notificationTest(FNotificatorId,changedKinds(0)|INotification::TestNotify);
}

void NotifyKindsWidget::onTestButtonClicked()
{
	onTestLinkActivated(QString::null);
}
