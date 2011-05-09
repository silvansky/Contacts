#include "simplevcarddialog.h"
#include "ui_simplevcarddialog.h"
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>
#include <utils/custominputdialog.h>

SimpleVCardDialog::SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars,
				     IStatusIcons *AStatusIcons, IStatusChanger * AStatusChanger,
				     IRosterPlugin *ARosterPlugin, IPresencePlugin *APresencePlugin,
				     IRosterChanger *ARosterChanger,
				     const Jid &AStreamJid, const Jid &AContactJid) :
		ui(new Ui::SimpleVCardDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_VCARD, 0, 0, "windowIcon");

	FContactJid = AContactJid;
	FStreamJid = AStreamJid;

	FAvatars = AAvatars;
	FStatusIcons = AStatusIcons;
	FStatusChanger = AStatusChanger;
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
	FVCard->unlock();
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

	if (FAvatars)
		FAvatars->insertAutoAvatar(ui->avatarLabel, FContactJid, QSize(48, 48), "pixmap");
	else
		ui->avatarLabel->clear();

	IPresenceItem presence = FPresence!=NULL ? FPresence->presenceItems(FContactJid).value(0) : IPresenceItem();
	ui->mood->setText(FRosterItem.isValid ? presence.status : tr("Not in contact list"));
	if (FStatusIcons)
		ui->status->setPixmap(FStatusIcons->iconByJidStatus(FContactJid, presence.show, SUBSCRIPTION_BOTH, false).pixmap(100));
	if (FStatusChanger)
		ui->statusText->setText(FStatusChanger->nameByShow(presence.show));

	QDate birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::ISODate);
	if (!birthday.isValid())
		birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::TextDate);
	QString birthdayString = birthday.isValid() ? birthday.toString(Qt::SystemLocaleLongDate) : "<font color=grey>" + tr("not assigned") + "</font>";
	ui->birthDateLabel->setText(birthdayString);

	// Временно скрываем комментарии из-за возможных проблем отображения
	/*
	QString remarkString = FVCard->value(VVN_DESCRIPTION);
	if (!remarkString.isEmpty())
	{
		ui->remarkLabel->setText(remarkString);
	}
	else
	*/
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
	CustomInputDialog * dialog = new CustomInputDialog(CustomInputDialog::String);
	dialog->setDefaultText(oldName);
	dialog->setCaptionText(tr("Rename contact"));
	dialog->setInfoText(tr("Enter new name"));
	connect(dialog, SIGNAL(stringAccepted(const QString&)), SLOT(onNewNameSelected(const QString&)));
	dialog->show();
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

void SimpleVCardDialog::on_editOnline_clicked()
{
	QDesktopServices::openUrl(QUrl("http://id.rambler.ru/script/settings.cgi"));
}
