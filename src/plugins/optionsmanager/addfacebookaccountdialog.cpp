#include "addfacebookaccountdialog.h"
#include "ui_addfacebookaccountdialog.h"

AddFacebookAccountDialog::AddFacebookAccountDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AddFacebookAccountDialog)
{
	ui->setupUi(this);
}

AddFacebookAccountDialog::~AddFacebookAccountDialog()
{
	delete ui;
}
