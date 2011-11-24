#include "growlpreferences.h"
#include "ui_growlpreferences.h"

GrowlPreferences::GrowlPreferences(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::GrowlPreferences)
{
	ui->setupUi(this);
    connect(ui->configureGrowlButton, SIGNAL(clicked()), SLOT(onGrowlSettingsButtonClicked()));
}

GrowlPreferences::~GrowlPreferences()
{
	delete ui;
}

void GrowlPreferences::onGrowlSettingsButtonClicked()
{
    emit showGrowlPreferences();
}
