#include "simplevcarddialog.h"
#include "ui_simplevcarddialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

SimpleVCardDialog::SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars, IStatusIcons *AStatusIcons, IRosterPlugin *ARosterPlugin, IPresencePlugin *APresencePlugin, IRosterChanger *ARosterChanger, const Jid &AStreamJid, const Jid &AContactJid) :
		ui(new Ui::SimpleVCardDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_VCARD, 0, 0, "windowIcon");

	FContactJid = AContactJid;
	FStreamJid = AStreamJid;
	FAvatars = AAvatars;
	FStatusIcons = AStatusIcons;
	FRosterChanger = ARosterChanger;

	FPresence = APresencePlugin->getPresence(FStreamJid);

	FVCard = AVCardPlugin->vcard(FContactJid);
	connect(FVCard->instance(), SIGNAL(vcardUpdated()), SLOT(onVCardUpdated()));
	connect(FVCard->instance(), SIGNAL(vcardError(const QString &)), SLOT(onVCardError(const QString &)));

	FRoster = ARosterPlugin->getRoster(AStreamJid);
	connect(FRoster->instance(), SIGNAL(received(const IRosterItem &, const IRosterItem &)),
		SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));

	FRosterItem = FRoster->rosterItem(FContactJid);
	if (FRosterItem.isValid)
		ui->addToRosterButton->setVisible(false);
	else
		ui->renameButton->setVisible(false);

	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	updateDialog();
	reloadVCard();
}

SimpleVCardDialog::~SimpleVCardDialog()
{
	delete ui;
}

Jid SimpleVCardDialog::streamJid() const
{
	return FStreamJid;
}

void SimpleVCardDialog::reloadVCard()
{
	FVCard->update(FStreamJid);
}

void SimpleVCardDialog::updateDialog()
{
	if (FRosterItem.isValid)
		ui->name->setText(FRosterItem.name);
	else
		ui->name->setText(FVCard->value(VVN_FULL_NAME).isEmpty() ? FContactJid.bare() : FVCard->value(VVN_FULL_NAME));
	setWindowTitle(tr("Profile: %1").arg(ui->name->text()));

	ui->jid->setText(FContactJid.bare());

	FAvatars->insertAutoAvatar(ui->avatarLabel, FContactJid, QSize(100, 100), "pixmap");

	IPresenceItem presence = FPresence->presenceItems(FContactJid).value(0);
	ui->mood->setText(FRosterItem.isValid ? presence.status : tr("Not in contact list"));
	ui->fullName->setText(FVCard->value(VVN_FULL_NAME));
	ui->status->setPixmap(FStatusIcons->iconByJidStatus(FContactJid, presence.show, SUBSCRIPTION_BOTH, false).pixmap(100));

	QDate birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::ISODate);
	if (!birthday.isValid())
		birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::TextDate);
	QString birthdayString = birthday.isValid() ? birthday.toString(Qt::SystemLocaleLongDate) : "<font color=grey>" + tr("not assigned") + "</font>";
	ui->birthDateLabel->setText(birthdayString);

	QString remarkString = FVCard->value(VVN_DESCRIPTION);
	if (!remarkString.isEmpty())
	{
		ui->remarkLabel->setText(remarkString);
	}
	else
	{
		ui->remarkCaption->setVisible(false);
		ui->remarkLabel->setVisible(false);
	}

	static const QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	QStringList phoneList = FVCard->values(VVN_TELEPHONE, phoneTagList).keys();
	ui->phoneLabel->setText(phoneList.isEmpty() ? "<font color=grey>" + tr("not assigned") + "</font>" : phoneList.join("<br>"));
}

void SimpleVCardDialog::onVCardUpdated()
{
	updateDialog();
}

void SimpleVCardDialog::onVCardError(const QString &AError)
{
	QMessageBox::critical(this, tr("vCard error"), tr("vCard request failed.<br>%1").arg(AError));
}

void SimpleVCardDialog::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid && FContactJid)
		FRosterItem = AItem;
	updateDialog();
}

void SimpleVCardDialog::on_renameButton_clicked()
{
	QString oldName = FRoster->rosterItem(FContactJid).name;
	QInputDialog * dialog = new QInputDialog;
	dialog->setStyleSheet(styleSheet());
	dialog->setTextValue(oldName);
	dialog->setWindowTitle(tr("Rename contact"));
	dialog->setLabelText(tr("<font size=+2>Rename contact</font><br>Enter new name"));
	connect(dialog, SIGNAL(textValueSelected(const QString&)), SLOT(onNewNameSelected(const QString&)));
	CustomBorderContainer * border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
	if (border)
	{
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setResizable(false);
		border->setWindowModality(Qt::ApplicationModal);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(dialog, SIGNAL(accepted()), border, SLOT(close()));
		connect(dialog, SIGNAL(rejected()), border, SLOT(close()));
		connect(border, SIGNAL(closeClicked()), dialog, SLOT(reject()));
		border->show();
	}
	else
	{
		dialog->setWindowModality(Qt::ApplicationModal);
		dialog->show();
	}
}

void SimpleVCardDialog::on_addToRosterButton_clicked()
{
	IAddContactDialog * dialog = NULL;
	QWidget * widget = FRosterChanger ? FRosterChanger->showAddContactDialog(FStreamJid) : NULL;
	if (widget)
	{
		if (!(dialog = qobject_cast<IAddContactDialog*>(widget)))
		{
			if (CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(widget))
				dialog = qobject_cast<IAddContactDialog*>(border->widget());
		}
		if (dialog)
			dialog->setContactJid(FContactJid.bare());
	}
}

void SimpleVCardDialog::onNewNameSelected(const QString & newName)
{
	QString oldName = FRoster->rosterItem(FContactJid).name;
	if (!newName.isEmpty() && newName != oldName)
	{
		FRoster->renameItem(FContactJid, newName);
		FRosterItem = FRoster->rosterItem(FContactJid);
	}
}
