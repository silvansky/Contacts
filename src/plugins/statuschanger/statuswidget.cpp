#include "statuswidget.h"
#include "ui_statuswidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <utils/iconstorage.h>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <QTextDocument>
#include <QWidgetAction>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>

#define DEFAULT_MOOD_TEXT "<i><font color=grey>Tell your friends about your mood</font></i>"
#define NO_AVATARS_HISTORY

StatusWidget::StatusWidget(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::StatusWidget)
{
	ui->setupUi(this);
	ui->statusToolButton->installEventFilter(this);
	ui->avatarLabel->installEventFilter(this);
	ui->avatarLabel->setAttribute(Qt::WA_Hover, true);
	avatarHovered = false;
	ui->moodEdit->setVisible(false);
	ui->moodEdit->installEventFilter(this);
	ui->moodLabel->installEventFilter(this);
	QString logoPath = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_SCHANGER_ROSTER_LOGO);
	logo.load(logoPath);
	profileMenu = new Menu();
	Action * manageProfileAction = new Action(profileMenu);
	manageProfileAction->setText(tr("Manage my profile"));
	profileMenu->addAction(manageProfileAction);
	connect(manageProfileAction, SIGNAL(triggered()), SLOT(onManageProfileTriggered()));
	Action * addAvatarAction = new Action(profileMenu);
	addAvatarAction->setText(tr("Add new photo..."));
	profileMenu->addAction(addAvatarAction);
	connect(addAvatarAction, SIGNAL(triggered()), SLOT(onAddAvatarTriggered()));
#ifndef NO_AVATARS_HISTORY
	//profileMenu->setBottomWidget(selectAvatarWidget);
	selectAvatarWidget = new SelectAvatarWidget(0);
	selectAvatarWidget->setAttribute(Qt::WA_DeleteOnClose, false);
	selectAvatarWidget->setWindowFlags(Qt::ToolTip);
	selectAvatarWidget->installEventFilter(this);
	connect(selectAvatarWidget, SIGNAL(avatarSelected(const QImage&)), SIGNAL(avatarChanged(const QImage&)));
	QWidgetAction * wa = new QWidgetAction(profileMenu);
	wa->setDefaultWidget(selectAvatarWidget);
	profileMenu->addWidgetActiion(wa);
#else
	selectAvatarWidget = 0;
#endif
	connect(profileMenu, SIGNAL(aboutToHide()), SLOT(profileMenuAboutToHide()));
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
/*
	if (!logo.isNull())
	{
		QPainter painter(this);
		painter.drawImage(rect().right() - logo.width(), 0, logo);
	}
*/
}

bool StatusWidget::eventFilter(QObject * obj, QEvent * event)
{
	if ((obj == ui->statusToolButton) && (event->type() == QEvent::ActionChanged))
	{
		QActionEvent * actionEvent = (QActionEvent*)event;
		if (actionEvent->action())
		{
			ui->statusToolButton->setIcon(actionEvent->action()->icon());
			ui->statusToolButton->setText(fitCaptionToWidth(userName, actionEvent->action()->text(), ui->statusToolButton->width() - ui->statusToolButton->iconSize().width() - 12));
			ui->statusToolButton->setToolTip(actionEvent->action()->text());
		}
		return true;
	}
	if ((obj == ui->statusToolButton) && (event->type() == QEvent::Resize))
	{
		ui->statusToolButton->setText(fitCaptionToWidth(userName, ui->statusToolButton->defaultAction()->text(), ui->statusToolButton->width() - ui->statusToolButton->iconSize().width() - 12));
		return true;
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
			if (avatarHovered || profileMenu->isVisible())
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
				int dx = selectAvatarWidget ? selectAvatarWidget->width() / 2 : profileMenu->sizeHint().width() / 2;
				point.setX(point.x() - dx);
				point.setY(point.y() + ui->avatarLabel->height());
				profileMenu->popup(point);
				break;
			}
		case QEvent::MouseButtonPress:
			{
				if (profileMenu->isVisible())
					profileMenu->hide();
			}
		default:
			break;
		}
	}
	//if ((obj == selectAvatarWidget) && (event->type() == QEvent::FocusOut))
	//	selectAvatarWidget->hide();
	if ((obj == ui->moodLabel) && (event->type() == QEvent::MouseButtonPress))
	{
		QMouseEvent * mouseEvent = (QMouseEvent*)event;
		if (mouseEvent->button() == Qt::LeftButton)
			startEditMood();
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
					finishEditMood();
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

void StatusWidget::profileMenuAboutToHide()
{
	avatarHovered = false;
}

void StatusWidget::onManageProfileTriggered()
{
	QDesktopServices::openUrl(QUrl("http://id-planet.rambler.ru/"));
}

void StatusWidget::onAddAvatarTriggered()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Select new avatar image"), "", tr("Image files %1").arg("(*.jpg *.bmp *.png)"));
	if (!filename.isEmpty())
	{
		QImage newAvatar;
		if (newAvatar.load(filename))
			emit avatarChanged(newAvatar);
	}
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
