#include "statuswidget.h"
#include "ui_statuswidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <utils/iconstorage.h>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <QTextDocument>

#define DEFAULT_MOOD_TEXT "<i><font color=grey>Tell your friends about your mood</font></i>"

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
	ui->moodEdit->setVisible(false);
	ui->moodEdit->installEventFilter(this);
	ui->moodLabel->installEventFilter(this);
	QString logoPath = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_ROSTER_LOGO);
	logo.load(logoPath);
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

void StatusWidget::paintEvent(QPaintEvent * event)
{
	QWidget::paintEvent(event);
	if (!logo.isNull())
	{
		QPainter painter(this);
		painter.drawImage(rect().right() - logo.width(), 0, logo);
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
			//ui->statusToolButton->setText("<b><font size=+2>" + userName + "</font></b> - <font size=-1>" + ui->statusToolButton->text() + "</font>");
			ui->statusToolButton->setText(fitCaptionToWidth(userName, ui->statusToolButton->text(), ui->statusToolButton->width() - ui->statusToolButton->iconSize().width() - 5));
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
	if ((obj == ui->moodLabel) && (event->type() == QEvent::MouseButtonPress))
	{
		QMouseEvent * mouseEvent = (QMouseEvent*)event;
		if (mouseEvent->button() == Qt::LeftButton)
		{
			startEditMood();
		}
	}
	if (obj == ui->moodEdit)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent * keyEvent = (QKeyEvent*)event;
			switch(keyEvent->key())
			{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				if (keyEvent->modifiers() == Qt::NoModifier)
				{
					finishEditMood();
				}
				break;
		case Qt::Key_Escape:
				cancelEditMood();
				return true;
				break;
			}
		}
		if (event->type() == QEvent::FocusOut)
			if (ui->moodEdit->isVisible())
				finishEditMood();
	}
	return QWidget::eventFilter(obj, event);
}

void StatusWidget::updateMoodText()
{
	if (userMood.length() <= 70)
		ui->moodLabel->setText(userMood.isEmpty() ? DEFAULT_MOOD_TEXT : userMood);
	else
		ui->moodLabel->setText(userMood.left(70) + "...");
}

void StatusWidget::onAvatarChanged(const QImage & img)
{
	//ui->avatarLabel->setPixmap(QPixmap::fromImage(img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

void StatusWidget::setUserName(const QString& name)
{
	userName = name;
}

void StatusWidget::setMoodText(const QString &mood)
{
	userMood = mood;
	updateMoodText();
}

void StatusWidget::startEditMood()
{
	ui->moodEdit->setText(userMood);
	ui->moodEdit->selectAll();
	ui->moodLabel->setVisible(false);
	ui->moodEdit->setVisible(true);
	ui->moodEdit->setFocus();
}

void StatusWidget::finishEditMood()
{
	userMood = ui->moodEdit->toPlainText();
	updateMoodText();
	ui->moodEdit->setVisible(false);
	ui->moodLabel->setVisible(true);
	ui->moodLabel->setFocus();
	emit moodSet(userMood);
}

void StatusWidget::cancelEditMood()
{
	ui->moodEdit->setVisible(false);
	ui->moodLabel->setVisible(true);
	ui->moodLabel->setFocus();
}

QString StatusWidget::fitCaptionToWidth(const QString & name, const QString & status, const int width) const
{
	QTextDocument doc;
	QString newName = name;
	const QString f1 = "<b><font size=+2>", f2 = "</font></b> - <font size=-1>", f3 = "</font>";
	doc.setHtml(f1 + name + f2 + status + f3);
	while ((doc.size().width() > width) && newName.length() > 1)
	{
		newName = newName.left(newName.length() - 1);
		doc.setHtml(f1 + newName  + "..." + f2 + status + f3);
	}
	return doc.toHtml();
}
