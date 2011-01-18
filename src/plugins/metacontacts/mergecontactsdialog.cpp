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

		QIcon avatar;
		QString name = !contact.name.isEmpty() ? contact.name : contact.id.node();
		
		if (ui.ltwContacts->count() == 0)
		{
			ui.lblAvatar->setPixmap(avatar.pixmap(avatar.availableSizes().value(0)));
			ui.lneName->setText(name);
		}
		
		QListWidgetItem *item = new QListWidgetItem(avatar,name);
		ui.ltwContacts->addItem(item);
	}

	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Merge contacts"));
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
}

MergeContactsDialog::~MergeContactsDialog()
{

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
