#ifndef ADDFACEBOOKACCOUNTDIALOG_H
#define ADDFACEBOOKACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class AddFacebookAccountDialog;
}

class AddFacebookAccountDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit AddFacebookAccountDialog(QWidget *parent = 0);
	~AddFacebookAccountDialog();
	
private:
	Ui::AddFacebookAccountDialog *ui;
};

#endif // ADDFACEBOOKACCOUNTDIALOG_H
