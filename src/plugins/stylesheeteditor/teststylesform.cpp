#include "teststylesform.h"
#include "ui_teststylesform.h"
#include <QMenu>
#include <QListView>
#include <utils/menu.h>

TestStylesForm::TestStylesForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TestStylesForm)
{
	ui->setupUi(this);
	Menu * menu = new Menu();
	Action * action;
	action = new Action();
	action->setText("Act1");
	menu->addAction(action);
	action = new Action();
	action->setText("Act2");
	menu->addAction(action);
	action = new Action();
	action->setText("Act3");
	menu->addAction(action);
	action = new Action();
	action->setText("Act4");
	menu->addAction(action);
	//menu->menuAction()->setIcon(QIcon(":/trolltech/qtgradienteditor/images/minus.png"));
	//menu->setDefaultAction(menu->actions().at(0));
	//menu->defaultAction()->setIcon(QIcon(":/trolltech/qtgradienteditor/images/minus.png"));
	//menu->menuAction()->setVisible(false);
	ui->popupBtn->addAction(menu->menuAction());
	ui->popupBtn->setDefaultAction(menu->menuAction());
	//ui->popupBtn->setIcon(QIcon(":/trolltech/qtgradienteditor/images/minus.png"));
	menu = new Menu();
	action = new Action();
	action->setText("Act1");
	menu->addAction(action);
	action = new Action();
	action->setText("Act2");
	menu->addAction(action);
	action = new Action();
	action->setText("Act3");
	menu->addAction(action);
	action = new Action();
	action->setText("Act4");
	menu->addAction(action);
	ui->menuBtn->setMenu(menu);
	ui->comboBox->setView(new QListView);
	ui->comboBox_2->setView(new QListView);
}

TestStylesForm::~TestStylesForm()
{
	delete ui;
}
