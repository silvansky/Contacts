#ifndef ADDRAMBLERACCOUNTDIALOG_H
#define ADDRAMBLERACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class AddRamblerAccountDialog;
}

class AddRamblerAccountDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit AddRamblerAccountDialog(QWidget *parent = 0);
	~AddRamblerAccountDialog();
	
private:
	Ui::AddRamblerAccountDialog *ui;
};

#endif // ADDRAMBLERACCOUNTDIALOG_H
