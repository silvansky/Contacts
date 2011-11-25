#include "mergecontactsdialog.h"

#include <QPushButton>
#include <utils/customborderstorage.h>
#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/graphicseffects.h>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

MergeContactsDialog::MergeContactsDialog(IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QList<QString> AMetaIds, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

#ifdef Q_WS_MAC
	ui.buttonsLayout->setSpacing(16);
	ui.buttonsLayout->addWidget(ui.pbtMerge);
	setWindowGrowButtonEnabled(this->window(), false);
#endif

	ui.lneName->setAttribute(Qt::WA_MacShowFocusRect, false);

	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;
	FMetaIds = AMetaIds;

	ui.lblNotice->setText(tr("These %n contacts will be merged into one:","",AMetaIds.count()));

	QString name;
	int nameCapCount = 0;
	QRegExp nameRegExp(tr("([a-z])","From first letter to last of alphabet"),Qt::CaseInsensitive);

	QSet<Jid> items;
	ui.ltContacts->addStretch();
	foreach(QString metaId, FMetaIds)
	{
		IMetaContact contact = FMetaRoster->metaContact(metaId);
		items += contact.items;

		QImage avatar = FMetaRoster->metaAvatarImage(metaId,false,false).scaled(24, 24, Qt::KeepAspectRatio,Qt::SmoothTransformation);
		QString itemName = FMetaContacts->metaContactName(contact);

		int pos = 0;
		int itemNameCapCount = 0;
		while ((pos = nameRegExp.indexIn(itemName, pos)) != -1) 
		{
			itemNameCapCount++;
			pos += nameRegExp.matchedLength();
		}
		if (nameCapCount < itemNameCapCount)
		{
			name = itemName;
			nameCapCount = itemNameCapCount;
		}
		else if (name.isEmpty())
		{
			name = itemName.trimmed();
		}

		QHBoxLayout *itemLayout = new QHBoxLayout();
		itemLayout->setContentsMargins(0, 0, 0, 0);
		itemLayout->setSpacing(8);

		QLabel *avatarLabel = new QLabel(this);
		avatarLabel->setFixedSize(24, 24);
		avatarLabel->setPixmap(QPixmap::fromImage(avatar));
		avatarLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		itemLayout->addWidget(avatarLabel);

		QLabel *nameLabel = new QLabel(this);
		nameLabel->setText(itemName);
		itemLayout->addWidget(nameLabel);

		ui.ltContacts->addItem(itemLayout);
	}
	ui.ltContacts->addStretch();
	ui.lneName->setText(name);

	QImage avatar;
	QMultiMap<int,Jid> orders = FMetaContacts->itemOrders(items.toList());
	for (QMultiMap<int,Jid>::const_iterator it=orders.constBegin(); avatar.isNull() && it!=orders.constEnd(); it++)
	{
		QString metaId = FMetaRoster->itemMetaContact(it.value());
		avatar = FMetaRoster->metaAvatarImage(metaId,true,false);
	}
	if (avatar.isNull())
		avatar = FMetaRoster->metaAvatarImage(FMetaIds.value(0),false,false);
	ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar.scaled(48, 48, Qt::KeepAspectRatio,Qt::SmoothTransformation)));

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (FBorder)
	{
		FBorder->setResizable(false);
		FBorder->setMinimizeButtonVisible(false);
		FBorder->setMaximizeButtonVisible(false);
		FBorder->setAttribute(Qt::WA_DeleteOnClose,true);
		FBorder->setWindowTitle(ui.lblCaption->text());
		connect(this, SIGNAL(accepted()), FBorder, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), FBorder, SLOT(closeWidget()));
		connect(FBorder, SIGNAL(closeClicked()), SLOT(reject()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	else
	{
		ui.lblCaption->setVisible(false);
		setAttribute(Qt::WA_DeleteOnClose,true);
	}

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_MERGECONTACTSDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	ui.lneName->selectAll();
	ui.lneName->setFocus();

	connect(ui.lneName,SIGNAL(textChanged(const QString &)),SLOT(onContactNameChanged(const QString &)));
	connect(ui.pbtCancel, SIGNAL(clicked()), SLOT(reject()));
	connect(ui.pbtMerge, SIGNAL(clicked()), SLOT(onAcceptButtonClicked()));
}

MergeContactsDialog::~MergeContactsDialog()
{
	if (FBorder)
		FBorder->deleteLater();
}

void MergeContactsDialog::show()
{
	if (FBorder)
	{
		// TODO: determine what of these are really needed
		FBorder->layout()->update();
		layout()->update();
		FBorder->adjustSize();
		FBorder->show();
		FBorder->layout()->update();
		FBorder->adjustSize();
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
