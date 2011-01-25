#include "mergecontactsdialog.h"

#include <QPushButton>

MergeContactsDialog::MergeContactsDialog(IMetaRoster *AMetaRoster, const QList<Jid> AMetaIds, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_MERGECONTACTSDIALOG);

	FMetaRoster = AMetaRoster;
	FMetaIds = AMetaIds;

	ui.lblNotice->setText(tr("<b>%n contacts</b> will be merged into one:","",AMetaIds.count()));

	foreach(Jid metaId, FMetaIds)
	{
		IMetaContact contact = FMetaRoster->metaContact(metaId);

		QImage avatar = FMetaRoster->metaAvatarImage(metaId).scaled(32,32,Qt::KeepAspectRatio,Qt::SmoothTransformation);
		QString name = !contact.name.isEmpty() ? contact.name : contact.id.node();
		
		if (ui.ltwContacts->count() == 0)
		{
			ui.lneName->setText(name);
			ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
		}
		
		QListWidgetItem *item = new QListWidgetItem(name);
		item->setData(Qt::DecorationRole, avatar);
		item->setData(Qt::UserRole, metaId.full());
		item->setFlags(Qt::ItemIsEnabled);
		ui.ltwContacts->addItem(item);
	}

	ui.sprSplitter->setSizes(QList<int>() << 400 << 500);

	connect(ui.lneName,SIGNAL(textChanged(const QString &)),SLOT(onContactNameChanged(const QString &)));

	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Merge contacts"));
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
}

MergeContactsDialog::~MergeContactsDialog()
{

}

void MergeContactsDialog::onContactNameChanged(const QString &AText)
{
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(!AText.trimmed().isEmpty());
}

void MergeContactsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (ui.dbbButtons->buttonRole(AButton))
	{
	case QDialogButtonBox::AcceptRole:
		if (!ui.lneName->text().isEmpty())
		{
			Jid parentId = FMetaIds.value(0);
			QList<Jid> childsId = FMetaIds.mid(1);
			if (FMetaRoster->metaContact(parentId).name != ui.lneName->text())
				FMetaRoster->renameContact(parentId,ui.lneName->text());
			FMetaRoster->mergeContacts(parentId,childsId);
			accept();
		}
		break;
	case QDialogButtonBox::RejectRole:
		reject();
		break;
	default:
		break;
	}
}
