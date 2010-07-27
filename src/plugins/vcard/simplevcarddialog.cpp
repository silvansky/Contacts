#include "simplevcarddialog.h"
#include "ui_simplevcarddialog.h"
#include <QMessageBox>

SimpleVCardDialog::SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars, IStatusIcons *AStatusIcons, IRosterPlugin * ARosterPlugin, IPresencePlugin * APresencePlugin, const Jid &AStreamJid, const Jid &AContactJid) :
		ui(new Ui::SimpleVCardDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("vCard - %1").arg(AContactJid.full()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_VCARD,0,0,"windowIcon");

	FContactJid = AContactJid;
	FStreamJid = AStreamJid;
	FVCardPlugin = AVCardPlugin;
	FAvatars = AAvatars;
	FStatusIcons = AStatusIcons;
	FRosterPlugin = ARosterPlugin;
	FVCard = FVCardPlugin->vcard(FContactJid);
	connect(FVCard->instance(),SIGNAL(vcardUpdated()),SLOT(onVCardUpdated()));
	connect(FVCard->instance(),SIGNAL(vcardError(const QString &)),SLOT(onVCardError(const QString &)));
	FRoster = FRosterPlugin->getRoster(AStreamJid);
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
		ui->name->setText(FVCard->value(VVN_FULL_NAME));
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
	QString birthdayString = birthday.toString(Qt::SystemLocaleLongDate);
	ui->birthDateLabel->setText(birthdayString);
	ui->remarkLabel->setText(FVCard->value(VVN_DESCRIPTION));
	static const QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	QHash<QString,QStringList> phones = FVCard->values(VVN_TELEPHONE, phoneTagList);
	QStringList list(phones.keys());
	ui->phoneLabel->setText(list.join("<br>"));
}

void SimpleVCardDialog::onVCardUpdated()
{
	updateDialog();
}

void SimpleVCardDialog::onVCardError(const QString &AError)
{
	QMessageBox::critical(this,tr("vCard error"),tr("vCard request or publish failed.<br>%1").arg(AError));
}
