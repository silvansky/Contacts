#ifndef MERGECONTACTSDIALOG_H
#define MERGECONTACTSDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imetacontacts.h>
#include <utils/stylestorage.h>
#include "ui_mergecontactsdialog.h"

class MergeContactsDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	MergeContactsDialog(IMetaRoster *AMetaRoster, const QList<Jid> AMetaIds, QWidget *AParent = NULL);
	~MergeContactsDialog();
protected slots:
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	Ui::MergeContactsDialog ui;
private:
	IMetaRoster *FMetaRoster;
private:
	QList<Jid> FMetaIds;
};

#endif // MERGECONTACTSDIALOG_H
