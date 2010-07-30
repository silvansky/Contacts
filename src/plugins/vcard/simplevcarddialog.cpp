#include "simplevcarddialog.h"
#include "ui_simplevcarddialog.h"
#include <QMessageBox>
#include <QInputDialog>

SimpleVCardDialog::SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars, IStatusIcons *AStatusIcons, IRosterPlugin * ARosterPlugin, IPresencePlugin * APresencePlugin, const Jid &AStreamJid, const Jid &AContactJid) :
		ui(new Ui::SimpleVCardDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_VCARD, 0, 0, "windowIcon");

	FContactJid = AContactJid;
	FStreamJid = AStreamJid;
	FVCardPlugin = AVCardPlugin;
	FAvatars = AAvatars;
	FStatusIcons = AStatusIcons;
	FRosterPlugin = ARosterPlugin;
	FVCard = FVCardPlugin->vcard(FContactJid);
	connect(FVCard->instance(), SIGNAL(vcardUpdated()), SLOT(onVCardUpdated()));
	connect(FVCard->instance(), SIGNAL(vcardError(const QString &)), SLOT(onVCardError(const QString &)));
	FRoster = FRosterPlugin->getRoster(AStreamJid);
	connect(FRoster->instance(), SIGNAL(received(const IRosterItem&)), SLOT(onRosterItemReceived(const IRosterItem&)));
	FRosterItem = FRoster->rosterItem(FContactJid);
	if (FRosterItem.isValid)
		ui->addToRosterButton->setVisible(false);
	else
		ui->renameButton->setVisible(false);
	FPresencePlugin = APresencePlugin;
	FPresence = FPresencePlugin->getPresence(FStreamJid);
	updateDialog();
	if (FVCard->isEmpty())
		reloadVCard();

	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));
}

SimpleVCardDialog::~SimpleVCardDialog()
{
	delete ui;
}

void SimpleVCardDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
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
	ui->jid->setText(FContactJid.bare());
	FAvatars->insertAutoAvatar(ui->avatarLabel, FContactJid, QSize(100, 100), "pixmap");
	QList<IPresenceItem> presenceItems = FPresence->presenceItems(FContactJid);
	IPresenceItem presence = presenceItems.isEmpty() ? *(new IPresenceItem) : presenceItems.first();
	ui->mood->setText(presence.status);
	ui->status->setPixmap(FStatusIcons->iconByJidStatus(FContactJid, presence.show, SUBSCRIPTION_BOTH, false).pixmap(100));
	ui->fullName->setText(FVCard->value(VVN_FULL_NAME));
	QDate birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::ISODate);
	if (!birthday.isValid())
		birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::TextDate);
	QString birthdayString = birthday.isValid() ? birthday.toString(Qt::SystemLocaleLongDate) : "<font color=grey>" + tr("not assigned") + "</font>";
	ui->birthDateLabel->setText(birthdayString);
	QString remarkString = FVCard->value(VVN_DESCRIPTION);
	if (!remarkString.isEmpty())
		ui->remarkLabel->setText(remarkString);
	else
	{
		ui->remarkCaption->setVisible(false);
		ui->remarkLabel->setVisible(false);
	}
	static const QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	QHash<QString,QStringList> phones = FVCard->values(VVN_TELEPHONE, phoneTagList);
	QStringList list(phones.keys());
	ui->phoneLabel->setText(list.isEmpty() ? "<font color=grey>" + tr("not assigned") + "</font>" : list.join("<br>"));
	setWindowTitle(tr("Profile: %1").arg(ui->name->text()));
}

void SimpleVCardDialog::onVCardUpdated()
{
	updateDialog();
}

void SimpleVCardDialog::onVCardError(const QString &AError)
{
	QMessageBox::critical(this, tr("vCard error"), tr("vCard request failed.<br>%1").arg(AError));
}

void SimpleVCardDialog::onRosterItemReceived(const IRosterItem &ARosterItem)
{
	if (ARosterItem.itemJid && FContactJid)
		FRosterItem = ARosterItem;
	updateDialog();
}

void SimpleVCardDialog::on_renameButton_clicked()
{
	QString oldName = FRoster->rosterItem(FContactJid).name;
	bool ok = false;
	QString newName = QInputDialog::getText(NULL, tr("Contact name"), tr("Enter name for contact"), QLineEdit::Normal, oldName, &ok);
	if (ok && !newName.isEmpty() && newName != oldName)
	{
		FRoster->renameItem(FContactJid, newName);
		FRosterItem = FRoster->rosterItem(FContactJid);
	}
}

void SimpleVCardDialog::on_addToRosterButton_clicked()
{
	FRoster->setItem(FContactJid, ui->name->text(), QSet<QString>());
}
