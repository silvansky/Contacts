#ifndef MERGECONTACTSDIALOG_H
#define MERGECONTACTSDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imetacontacts.h>
#include <utils/stylestorage.h>
#include "ui_mergecontactsdialog.h"

class CustomBorderContainer;

class MergeContactsDialog :
	public QDialog
{
	Q_OBJECT
public:
	MergeContactsDialog(IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QList<QString> AMetaIds, QWidget *AParent = NULL);
	~MergeContactsDialog();
	void show();
protected slots:
	void onContactNameChanged(const QString &AText);
	void onAcceptButtonClicked();
private:
	Ui::MergeContactsDialog ui;
private:
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
private:
	QList<QString> FMetaIds;
	CustomBorderContainer *FBorder;
};

#endif // MERGECONTACTSDIALOG_H
