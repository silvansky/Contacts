#include "profilewidget.h"
#include "ui_profilewidget.h"

#include <utils/imagemanager.h>

#include <QStyleOption>
#include <QPainter>

ProfileWidget::ProfileWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ProfileWidget)
{
	ui->setupUi(this);
	connect(ui->deleteButton, SIGNAL(clicked()), SIGNAL(removeButtonClicked()));
}

ProfileWidget::~ProfileWidget()
{
	delete ui;
}

QString ProfileWidget::name() const
{
	return _name;
}

void ProfileWidget::setName(const QString &newName)
{
	_name = newName;
	ui->name->setText(_name);
}

Jid ProfileWidget::jid() const
{
	return _jid;
}

void ProfileWidget::setJid(const Jid &newJid)
{
	_jid = newJid;
}

QImage ProfileWidget::photo() const
{
	return _photo;
}

void ProfileWidget::setPhoto(const QImage &newPhoto)
{
	_photo = newPhoto;
	QImage fitting = ImageManager::roundSquared(_photo, 36, 3, true);
	ui->avatar->setPixmap(QPixmap::fromImage(fitting));
}

void ProfileWidget::paintEvent(QPaintEvent *pe)
{
	Q_UNUSED(pe)
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ProfileWidget::mousePressEvent(QMouseEvent *me)
{
	QWidget::mousePressEvent(me);
	emit clicked();
}
