#include "customstatusdialog.h"

#include <QList>
#include <QPushButton>
#include <QListView>

#define MAX_CHARACTERS  140
#define LOW_CHARACTERS  10

CustomStatusDialog::CustomStatusDialog(IStatusChanger *AStatusChanger, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	ui.lneStatusName->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.cmbStatusShow->setView(new QListView);
	setAttribute(Qt::WA_DeleteOnClose, true);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SCHANGER_CUSTOMSTATUSDIALOG);

	FStatusChanger = AStatusChanger;
	FStreamJid = AStreamJid;

	const QList<int> showList = QList<int>() << IPresence::Online << IPresence::Away << IPresence::DoNotDisturb;
	foreach(int show, showList)
		ui.cmbStatusShow->addItem(FStatusChanger->iconByShow(show),FStatusChanger->nameByShow(show),show);

	ui.lblCharsLeft->setVisible(false);

	connect(ui.lneStatusName,SIGNAL(textChanged(const QString &)),SLOT(onStatusNameChanged()));
	connect(ui.pteStatusText,SIGNAL(textChanged()),SLOT(onStatusTextChanged()));
	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogButtonAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));

	updateButtonsState();
}

CustomStatusDialog::~CustomStatusDialog()
{

}

void CustomStatusDialog::updateButtonsState()
{
	int nameCharsCount = ui.lneStatusName->text().trimmed().count();
	int textCharsCount = ui.pteStatusText->toPlainText().length();
	bool okEnabled = textCharsCount>0 && textCharsCount<=MAX_CHARACTERS;
	okEnabled = okEnabled && nameCharsCount>0;
	okEnabled = okEnabled && FStatusChanger->statusByName(ui.lneStatusName->text().trimmed())==STATUS_NULL_ID;
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

void CustomStatusDialog::onStatusNameChanged()
{
	updateButtonsState();
}

void CustomStatusDialog::onStatusTextChanged()
{
	int charsLeft = MAX_CHARACTERS - ui.pteStatusText->toPlainText().length();
	if (charsLeft > LOW_CHARACTERS)
		ui.lblCharsLeft->setText(tr("%1 characters left").arg(charsLeft));
	else
		ui.lblCharsLeft->setText(tr("<span style='color: red;'>%1</span> characters left").arg(charsLeft));
	ui.lblCharsLeft->setVisible(charsLeft < MAX_CHARACTERS);
	updateButtonsState();
}

void CustomStatusDialog::onDialogButtonAccepted()
{
	QString statusName = ui.lneStatusName->text().trimmed();
	int statusShow = ui.cmbStatusShow->itemData(ui.cmbStatusShow->currentIndex()).toInt();
	QString statusText = ui.pteStatusText->toPlainText().trimmed();
	int statusPriority = FStatusChanger->statusItemPriority(FStatusChanger->statusByShow(statusShow).value(0));
	int statusId = FStatusChanger->addStatusItem(statusName,statusShow,statusText,statusPriority);
	FStatusChanger->setStreamStatus(FStreamJid,statusId);
	accept();
}
