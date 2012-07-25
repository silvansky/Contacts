#include "welcomescreenwidget.h"
#include "ui_welcomescreenwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <utils/stylestorage.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/textflags.h>

WelcomeScreenWidget::WelcomeScreenWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::WelcomeScreenWidget)
{
	ui->setupUi(this);
	ui->address->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui->add->addTextFlag(TF_LIGHTSHADOW);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_RCHANGER_WELCOMESCREEN);

	connect(ui->add, SIGNAL(clicked()), SLOT(onAddPressed()));
	connect(ui->address, SIGNAL(textChanged(const QString &)), SLOT(onTextChanged(const QString &)));

	ui->address->installEventFilter(this);
	
	registerButtonsLayout = new QGridLayout();
  registerButtonsLayout->setSpacing(2);
  registerButtonsLayout->setObjectName(QString::fromUtf8("layout"));
  registerButtonsLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
  ui->horizontalLayout_2->addLayout(registerButtonsLayout);
}

WelcomeScreenWidget::~WelcomeScreenWidget()
{
	delete ui;
}

void WelcomeScreenWidget::paintEvent(QPaintEvent * pe)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(pe->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool WelcomeScreenWidget::eventFilter(QObject * obj, QEvent * evt)
{
	if (obj == ui->address)
	{
		if (evt->type() == QEvent::KeyPress)
		{
			QKeyEvent * ke = (QKeyEvent*)evt;
			if ((ke->key() == Qt::Key_Enter) || (ke->key() == Qt::Key_Return))
				onAddPressed();
		}
	}
	return QWidget::eventFilter(obj, evt);
}

void WelcomeScreenWidget::onAddPressed()
{
	emit addressEntered(ui->address->text());
	ui->address->setText(QString::null);
}

void WelcomeScreenWidget::onTextChanged(const QString & text)
{
	ui->add->setEnabled(!text.isEmpty());
}
