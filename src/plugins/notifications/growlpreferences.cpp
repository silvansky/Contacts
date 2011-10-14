#include "growlpreferences.h"
#include "ui_growlpreferences.h"

GrowlPreferences::GrowlPreferences(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::GrowlPreferences)
{
	ui->setupUi(this);
}

GrowlPreferences::~GrowlPreferences()
{
	delete ui;
}

void GrowlPreferences::on_lblSettings_linkActivated(const QString &link)
{
	if (link == "growl.preferences")
	{
		emit showGrowlPreferences();
	}
}
