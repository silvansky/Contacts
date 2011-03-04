#include "teststylesform.h"
#include "ui_teststylesform.h"
#include <QMenu>
#include <QListView>
#include <utils/menu.h>
#include <utils/imagemanager.h>

TestStylesForm::TestStylesForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TestStylesForm)
{
	ui->setupUi(this);

	ui->lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui->verticalScrollBar_2->setEnabled(false);

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
	//QImage img = ImageManager::grayscaled(ui->label->pixmap()->toImage());
	QImage src = ui->label->pixmap()->toImage();
	QImage img = ImageManager::colorized(src, QColor(200, 30, 40));
	ui->label->setPixmap(QPixmap::fromImage(img));
	img = ImageManager::colorized(src, QColor(20, 230, 40));
	ui->label_2->setPixmap(QPixmap::fromImage(img));
	img = ImageManager::colorized(src, QColor(0, 30, 240));
	ui->label_3->setPixmap(QPixmap::fromImage(img));
	img = ImageManager::grayscaled(src);
	ui->label_4->setPixmap(QPixmap::fromImage(img));
	img = ImageManager::squared(src, 140);
	ui->label_5->setPixmap(QPixmap::fromImage(img));
	img = ImageManager::colorized(src, QColor(200, 200, 200));
	ui->label_6->setPixmap(QPixmap::fromImage(img));
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
