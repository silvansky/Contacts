#include "addrambleraccountdialog.h"
#include "ui_addrambleraccountdialog.h"

AddRamblerAccountDialog::AddRamblerAccountDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AddRamblerAccountDialog)
{
	ui->setupUi(this);
}

AddRamblerAccountDialog::~AddRamblerAccountDialog()
{
	delete ui;
}
