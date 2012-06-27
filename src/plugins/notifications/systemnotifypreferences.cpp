#include "systemnotifypreferences.h"
#include "ui_systemnotifypreferences.h"

SystemNotifyPreferences::SystemNotifyPreferences(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SystemNotifyPreferences)
{
	ui->setupUi(this);
	connect(ui->configureButton, SIGNAL(clicked()), SLOT(onSettingsButtonClicked()));
}

SystemNotifyPreferences::~SystemNotifyPreferences()
{
	delete ui;
}

void SystemNotifyPreferences::onSettingsButtonClicked()
{
    emit showPreferences();
}
