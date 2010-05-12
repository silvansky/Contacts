#include "statuswidget.h"
#include "ui_statuswidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

StatusWidget::StatusWidget(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::StatusWidget)
{
	ui->setupUi(this);
	ui->statusToolButton->installEventFilter(this);
	ui->avatarLabel->installEventFilter(this);
	ui->avatarLabel->setAttribute(Qt::WA_Hover, true);
	avatarHovered = false;
	selectAvatarWidget = new SelectAvatarWidget(0);
	selectAvatarWidget->setAttribute(Qt::WA_DeleteOnClose, false);
	selectAvatarWidget->setWindowFlags(Qt::ToolTip);
	selectAvatarWidget->installEventFilter(this);
	connect(selectAvatarWidget, SIGNAL(avatarSelected(const QImage&)), SIGNAL(avatarChanged(const QImage&)));
}

StatusWidget::~StatusWidget()
{
	delete ui;
}

void StatusWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

bool StatusWidget::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == ui->statusToolButton)
	{
		if (ui->statusToolButton->defaultAction())
		{
			//			ui->statusLabel->setPixmap(ui->statusToolButton->defaultAction()->icon().pixmap(16, 16));
			ui->statusToolButton->setText(ui->statusToolButton->defaultAction()->text());
			ui->statusToolButton->setText(userName + " - " + ui->statusToolButton->text());
		}
	}
	if (obj == ui->avatarLabel)
	{
		switch ((int)event->type())
		{
		case QEvent::HoverEnter:
			avatarHovered = true;
			break;
		case QEvent::HoverLeave:
			avatarHovered = false;
			break;
		case QEvent::Paint:
			if (avatarHovered || selectAvatarWidget->isVisible())
			{
				QPaintEvent *paintEvent = (QPaintEvent*)event;
				QPainter painter(ui->avatarLabel);
				QIcon * icon = 0;
				if (ui->avatarLabel->pixmap())
				{
					icon = new QIcon((*(ui->avatarLabel->pixmap())));
					icon->paint(&painter, paintEvent->rect(), Qt::AlignCenter, QIcon::Selected, QIcon::On);
					delete icon;
				}
				painter.setPen(QColor::fromRgb(0, 0, 255, 50));
				QRect rect = paintEvent->rect();
				rect.setWidth(rect.width() - 1);
				rect.setHeight(rect.height() - 1);
				painter.drawRect(rect);
				QPolygon triangle;
				triangle.append(QPoint(0, 0));
				triangle.append(QPoint(6, 0));
				triangle.append(QPoint(3, 3));
				painter.translate(QPoint(paintEvent->rect().width() / 2 - 3, paintEvent->rect().height() - 5));
				painter.setPen(Qt::white);
				painter.setBrush(QBrush(Qt::black, Qt::SolidPattern));
				painter.drawPolygon(triangle, Qt::OddEvenFill);
				return true;
			}
			break;
		case QEvent::MouseButtonRelease:
			{
				QPoint point = mapToGlobal(ui->avatarLabel->pos());
				point.setX(point.x() - selectAvatarWidget->width() / 2);
				point.setY(point.y() + ui->avatarLabel->height());
				selectAvatarWidget->move(point);
				if (!selectAvatarWidget->isVisible())
				{
					selectAvatarWidget->show();
					selectAvatarWidget->activateWindow();
					selectAvatarWidget->setFocus(Qt::MouseFocusReason);
				}
				else
					selectAvatarWidget->hide();
				break;
			}
		default:
			break;
		}
	}
	if (obj == selectAvatarWidget && event->type() == QEvent::FocusOut)
		selectAvatarWidget->hide();
	return QWidget::eventFilter(obj, event);
}

void StatusWidget::onAvatarChanged(const QImage & img)
{
	ui->avatarLabel->setPixmap(QPixmap::fromImage(img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

void StatusWidget::setUserName(const QString& name)
{
	userName = name;
}
