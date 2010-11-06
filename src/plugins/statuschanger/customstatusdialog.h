#ifndef CUSTOMSTATUSDIALOG_H
#define CUSTOMSTATUSDIALOG_H

#include <QDialog>
#include <definations/resources.h>
#include <definations/stylesheets.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ipresence.h>
#include <utils/stylestorage.h>
#include "ui_customstatusdialog.h"

class CustomStatusDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	CustomStatusDialog(IStatusChanger *AStatusChanger, const Jid &AStreamJid, QWidget *AParent = NULL);
	~CustomStatusDialog();
protected:
	void updateButtonsState();
protected slots:
	void onStatusNameChanged();
	void onStatusTextChanged();
	void onDialogButtonAccepted();
private:
	Ui::CustomStatusDialogClass ui;
private:
	IStatusChanger *FStatusChanger;
private:
	Jid FStreamJid;
};

#endif // CUSTOMSTATUSDIALOG_H
