#include "addaccountwidget.h"
#include "ui_addaccountwidget.h"

AddAccountWidget::AddAccountWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AddAccountWidget)
{
	ui->setupUi(this);
}

AddAccountWidget::~AddAccountWidget()
{
	delete ui;
}
