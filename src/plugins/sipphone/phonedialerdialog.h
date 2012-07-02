#ifndef PHONEDIALERDIALOG_H
#define PHONEDIALERDIALOG_H

#include <QDialog>
#include <QSignalMapper>
#include <interfaces/isipphone.h>
#include "ui_phonedialerdialog.h"

class PhoneDialerDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	PhoneDialerDialog(ISipManager *ASipManager, QWidget *AParent = NULL);
	~PhoneDialerDialog();
protected slots:
	void onButtonMapped(const QString &AText);
	void onNumberTextChanged(const QString &AText);
private:
	Ui::PhoneDialerDialogClass ui;
private:
	ISipManager *FSipManager;
private:
	QSignalMapper FMapper;
};

#endif // PHONEDIALERDIALOG_H
