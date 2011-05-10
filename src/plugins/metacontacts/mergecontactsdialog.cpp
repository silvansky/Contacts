#include "mergecontactsdialog.h"

#include <QPushButton>

#include <utils/customborderstorage.h>
#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/graphicseffects.h>

MergeContactsDialog::MergeContactsDialog(IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QList<QString> AMetaIds, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	ui.lneName->setAttribute(Qt::WA_MacShowFocusRect, false);

	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;
	FMetaIds = AMetaIds;

	ui.lblNotice->setText(tr("These %n contacts will be merged into one:","",AMetaIds.count()));

	ui.ltContacts->addStretch();

	foreach(QString metaId, FMetaIds)
	{
		IMetaContact contact = FMetaRoster->metaContact(metaId);

		QImage avatar = FMetaRoster->metaAvatarImage(metaId,false,false).scaled(24, 24, Qt::KeepAspectRatio,Qt::SmoothTransformation);
		QString name = FMetaContacts->metaContactName(contact);

		if (ui.ltContacts->count() == 1)
		{
			ui.lneName->setText(name);
			ui.lblAvatar->setPixmap(QPixmap::fromImage(FMetaRoster->metaAvatarImage(metaId,false,false).scaled(48, 48, Qt::KeepAspectRatio,Qt::SmoothTransformation)));
		}

		QHBoxLayout * itemLayout = new QHBoxLayout();
		itemLayout->setContentsMargins(0, 0, 0, 0);
		itemLayout->setSpacing(8);
		QLabel * avatarLabel = new QLabel(this);
		avatarLabel->setFixedSize(24, 24);
		avatarLabel->setPixmap(QPixmap::fromImage(avatar));
		QLabel * nameLabel = new QLabel(this);
		avatarLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		nameLabel->setText(name);
		itemLayout->addWidget(avatarLabel);
		itemLayout->addWidget(nameLabel);
		ui.ltContacts->addItem(itemLayout);
	}

	ui.ltContacts->addStretch();

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
		border->setWindowTitle(ui.lblCaption->text());
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(border, SIGNAL(closeClicked()), SIGNAL(rejected()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose,true);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_MERGECONTACTSDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	ui.lneName->setFocus();

	connect(ui.lneName,SIGNAL(textChanged(const QString &)),SLOT(onContactNameChanged(const QString &)));

	connect(ui.pbtMerge, SIGNAL(clicked()), SLOT(onAcceptButtonClicked()));
}

MergeContactsDialog::~MergeContactsDialog()
{
	if (border)
		border->deleteLater();
}

CustomBorderContainer * MergeContactsDialog::windowBorder() const
{
	return border;
}

void MergeContactsDialog::show()
{
	if (border)
	{
		// TODO: determine what of these are really needed
		border->layout()->update();
		layout()->update();
		border->adjustSize();
		border->show();
		border->layout()->update();
		border->adjustSize();
	}
	else
		QDialog::show();
}

void MergeContactsDialog::onContactNameChanged(const QString &AText)
{
	ui.pbtMerge->setEnabled(!AText.trimmed().isEmpty());
}

void MergeContactsDialog::onAcceptButtonClicked()
{
	if (!ui.lneName->text().isEmpty())
	{
		QString parentId = FMetaIds.value(0);
		QList<QString> childsId = FMetaIds.mid(1);
		if (FMetaRoster->metaContact(parentId).name != ui.lneName->text())
			FMetaRoster->renameContact(parentId,ui.lneName->text());
		FMetaRoster->mergeContacts(parentId,childsId);
		accept();
	}
}
