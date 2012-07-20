#include "notifykindswidget.h"

NotifyKindsWidget::NotifyKindsWidget(INotifications *ANotifications, const QString &ATypeId, const QString &ATitle, ushort AKindMask, ushort AKindDefs, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.lblTitle->setText(ATitle);
	ui.lblTest->setVisible(false);

	FNotifications = ANotifications;
	FTypeId = ATypeId;
	FKindMask = AKindMask;
	FKindDefs = AKindDefs;

	ui.chbPopup->setEnabled(AKindMask & INotification::PopupWindow);
	ui.chbSound->setEnabled(AKindMask & INotification::SoundPlay);

	connect(this, SIGNAL(modified()), SLOT(onModified()));
	connect(this, SIGNAL(childReset()), SLOT(onModified()));

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
	FNotifications->setTypeNotificationKinds(FTypeId,changedKinds(FKindDefs));
	emit childApply();
}

void NotifyKindsWidget::reset()
{
	ushort kinds = FNotifications->typeNotificationKinds(FTypeId);
	ui.chbPopup->setChecked(kinds & INotification::PopupWindow);
	ui.chbSound->setChecked(kinds & INotification::SoundPlay);
	emit childReset();
}

ushort NotifyKindsWidget::changedKinds(ushort AActiveKinds) const
{
	ushort kinds = AActiveKinds;

	if (ui.chbPopup->isChecked())
		kinds |= INotification::PopupWindow;
	else
		kinds &= ~INotification::PopupWindow;

	if (ui.chbSound->isChecked())
		kinds |= INotification::SoundPlay;
	else
		kinds &= ~INotification::SoundPlay;

	return kinds;
}

void NotifyKindsWidget::onTestLinkActivated(const QString &ALink)
{
	Q_UNUSED(ALink);
	emit notificationTest(FTypeId,changedKinds(0));
}

void NotifyKindsWidget::onTestButtonClicked()
{
	onTestLinkActivated(QString::null);
}

void NotifyKindsWidget::onModified()
{
	bool on = ui.chbPopup->isChecked() || ui.chbSound->isChecked();
	ui.pbtTest->setEnabled(on);
	ui.pbtTest->setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor);
}
