#include "statuswidget.h"
#include "ui_statuswidget.h"

#include <QtDebug>

#include <QUrl>
#include <QPainter>
#include <QFileDialog>
#include <QPaintEvent>
#include <QTextDocument>
#include <QWidgetAction>
#include <QDesktopServices>
#include <utils/imagemanager.h>
#ifdef Q_WS_WIN
# include <utils/nonmodalopenfiledialog.h>
#endif

#define MAX_CHARACTERS  140

StatusWidget::StatusWidget(IStatusChanger *AStatusChanger, IAvatars *AAvatars, IVCardPlugin *AVCardPlugin, IMainWindowPlugin* AMainWindowPlugin, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

#ifdef Q_WS_MAC
	ui.lblName->setVisible(false);
	QLayoutItem * spacer = ui.avatarLt->itemAt(0);
	ui.avatarLt->removeItem(spacer);
	ui.avatarLt->addItem(spacer);
	ui.mainLt->removeItem(ui.avatarLt);
	ui.mainLt->removeItem(ui.nameMoodLt);
	ui.mainLt->removeItem(ui.statusLt);
	ui.mainLt->insertLayout(0, ui.avatarLt);
	ui.mainLt->insertLayout(1, ui.nameMoodLt);
	ui.mainLt->insertLayout(2, ui.statusLt);
	layout()->setContentsMargins(2, 0, 2, 0);
	ui.avatarLt->setContentsMargins(0, 4, 0, 0);
	ui.statusLt->setContentsMargins(0, 6, 0, 0);
	ui.nameMoodLt->setContentsMargins(0, -10, 0, 0);
#endif

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SCHANGER_STATUSWIDGET);

	FAvatars = AAvatars;
	FVCardPlugin = AVCardPlugin;
	FStatusChanger = AStatusChanger;
	FMainWindowPlugin = AMainWindowPlugin;

	FAvatarHovered = false;
	FSelectAvatarWidget = NULL;

	ui.lblName->setElideMode(Qt::ElideRight);
	ui.lblMood->setElideMode(Qt::ElideRight);

	ui.lblMood->setMultilineElideEnabled(true);

	ui.lblAvatar->setProperty("ignoreFilter", true);
	ui.lblMood->setProperty("ignoreFilter", true);

	ui.tlbStatus->installEventFilter(this);
	ui.lblAvatar->installEventFilter(this);
	ui.tedMood->installEventFilter(this);
	ui.lblMood->installEventFilter(this);

	ui.tedMood->setVisible(false);
	ui.tedMood->setMinimumLines(1);
	ui.tedMood->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.tedMood->setAutoResize(false);
	ui.lblAvatar->setAttribute(Qt::WA_Hover, true);
	ui.lblAvatar->setMouseTracking(true);

	ui.tlbStatus->addAction(FStatusChanger->statusMenu()->menuAction());
	ui.tlbStatus->setDefaultAction(FStatusChanger->statusMenu()->menuAction());

	FProfileMenu = new Menu();
	connect(FProfileMenu, SIGNAL(aboutToHide()), SLOT(onProfileMenuAboutToHide()));
	connect(FProfileMenu, SIGNAL(aboutToShow()), SLOT(onProfileMenuAboutToShow()));

	Action *manageProfileAction = new Action(FProfileMenu);
	manageProfileAction->setText(tr("My account"));
	connect(manageProfileAction, SIGNAL(triggered()), SLOT(onManageProfileTriggered()));
	FProfileMenu->addAction(manageProfileAction);

	if (FAvatars)
	{
		Action *addAvatarAction = new Action(FProfileMenu);
		addAvatarAction->setText(tr("Add new photo..."));
		connect(addAvatarAction, SIGNAL(triggered()), SLOT(onAddAvatarTriggered()));
		FProfileMenu->addAction(addAvatarAction);

		/*
		QWidgetAction *widgetAction = new QWidgetAction(FProfileMenu);
		FSelectAvatarWidget = new SelectAvatarWidget(FProfileMenu);
		widgetAction->setDefaultWidget(FSelectAvatarWidget);
		FProfileMenu->addWidgetActiion(widgetAction);
		*/
	}

	if (FVCardPlugin)
		connect(FVCardPlugin->instance(), SIGNAL(vcardReceived(const Jid &)), SLOT(onVCardReceived(const Jid &)));
	connect(FStatusChanger->instance(),SIGNAL(statusChanged(const Jid &, int)),SLOT(onStatusChanged(const Jid &, int)));
}

StatusWidget::~StatusWidget()
{

}

Jid StatusWidget::streamJid() const
{
	return FStreamJid;
}

void StatusWidget::setStreamJid(const Jid &AStreamJid)
{
	FStreamJid = AStreamJid;
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblAvatar, FStreamJid, QSize(24, 24), "pixmap");
	if (FVCardPlugin)
		onVCardReceived(FStreamJid);
}

void StatusWidget::startEditMood()
{
	ui.lblMood->setVisible(false);
	ui.tedMood->setVisible(true);
	ui.tedMood->setText(FUserMood);
	ui.tedMood->setFocus();
	ui.tedMood->selectAll();
}

void StatusWidget::finishEditMood()
{
	ui.tedMood->setVisible(false);
	ui.lblMood->setVisible(true);
	ui.lblMood->setFocus();

	QString mood = ui.tedMood->toPlainText().left(MAX_CHARACTERS).trimmed();
	foreach(int statusId, FStatusChanger->statusItems())
		if (statusId > STATUS_NULL_ID)
			FStatusChanger->updateStatusItem(statusId,FStatusChanger->statusItemName(statusId),FStatusChanger->statusItemShow(statusId),mood,FStatusChanger->statusItemPriority(statusId));
}

void StatusWidget::cancelEditMood()
{
	ui.tedMood->setVisible(false);
	ui.lblMood->setVisible(true);
	ui.lblMood->setFocus();
}

void StatusWidget::setUserName(const QString &AName)
{
	FUserName = AName;
	//ui.tlbStatus->setText(fitCaptionToWidth(FUserName, ui.tlbStatus->defaultAction()->text(), ui.tlbStatus->width() - ui.tlbStatus->iconSize().width() - 12));
#ifdef Q_WS_MAC
	window()->setWindowTitle(AName);
#else
	ui.lblName->setText(AName);
#endif
}

void StatusWidget::setMoodText(const QString &AMood)
{
	FUserMood = AMood;
	ui.lblMood->setText(AMood.isEmpty() ? tr("Tell your friends about your mood") : AMood);
}

QString StatusWidget::fitCaptionToWidth(const QString &AName, const QString &AStatus, const int AWidth) const
{
	QTextDocument doc;
	QString newName = AName;
	const QString f1 = "<b><font size=+2>", f2 = "</font></b> - <font size=-1>", f3 = "</font>";
	doc.setHtml(f1 + AName + f2 + AStatus + f3);
	while ((doc.size().width() > AWidth) && newName.length() > 1)
	{
		newName = newName.left(newName.length() - 1);
		doc.setHtml(f1 + newName  + "..." + f2 + AStatus + f3);
	}
	return getHtmlBody(doc.toHtml());
}

void StatusWidget::resizeEvent(QResizeEvent * event)
{
	if (FMainWindowPlugin && FMainWindowPlugin->mainWindowBorder())
	{
		QPoint p = ui.tlbStatus->geometry().topRight();
		p = mapToGlobal(p);
		p = FMainWindowPlugin->mainWindowBorder()->widget()->mapFromGlobal(p);
		//FMainWindowPlugin->mainWindowBorder()->setHeaderMoveLeft(p.x() + 1);
	}
	QWidget::resizeEvent(event);
}

void StatusWidget::paintEvent(QPaintEvent * pe)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(pe->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool StatusWidget::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if ((AObject == ui.tlbStatus) && (AEvent->type() == QEvent::ActionChanged))
	{
		QActionEvent *actionEvent = static_cast<QActionEvent*>(AEvent);
		if (actionEvent && actionEvent->action())
		{
			ui.tlbStatus->setIcon(actionEvent->action()->icon());
			//ui.tlbStatus->setText(fitCaptionToWidth(FUserName, actionEvent->action()->text(), ui.tlbStatus->width() - ui.tlbStatus->iconSize().width() - 12));
			ui.tlbStatus->setToolTip(actionEvent->action()->text());
			return true;
		}
	}
	else if ((AObject == ui.tlbStatus) && (AEvent->type() == QEvent::Resize))
	{
		//ui.tlbStatus->setText(fitCaptionToWidth(FUserName, ui.tlbStatus->defaultAction()->text(), ui.tlbStatus->width() - ui.tlbStatus->iconSize().width() - 12));
	}
	else if (AObject == ui.lblAvatar)
	{
		switch ((int)AEvent->type())
		{
		/*case QEvent::HoverEnter:
			FAvatarHovered = true;
			break;
		case QEvent::HoverLeave:
			FAvatarHovered = false;
			break;
		case QEvent::Paint:
			if (FAvatarHovered || FProfileMenu->isVisible())
			{
				QPaintEvent *paintEvent = (QPaintEvent*)AEvent;
				QPainter painter(ui.lblAvatar);
				QIcon * icon = 0;
				if (ui.lblAvatar->pixmap())
				{
					icon = new QIcon((*(ui.lblAvatar->pixmap())));
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
			break;*/
		case QEvent::MouseButtonRelease:
			{
				QPoint point = ui.lblAvatar->mapToGlobal(QPoint(0, 0));
				int dx = 0;//FSelectAvatarWidget ? FSelectAvatarWidget->width() / 2 : FProfileMenu->sizeHint().width() / 2;
				point.setX(point.x() - dx);
				point.setY(point.y() + ui.lblAvatar->height());
				FProfileMenu->popup(point);
				return true;
			}
			break;
		case QEvent::MouseButtonPress:
			return true;
			break;
		default:
			break;
		}
	}
	else if ((AObject == ui.lblMood) && (AEvent->type() == QEvent::MouseButtonPress))
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(AEvent);
		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
			startEditMood();
		return true;
	}
	else if (AObject == ui.tedMood)
	{
		if (AEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
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
			default:
				QChar keyChar(keyEvent->key());
				if (keyChar.isPrint() && ui.tedMood->toPlainText().length()>=MAX_CHARACTERS)
					return true;
				break;
			}
		}
		else if (AEvent->type() == QEvent::FocusOut)
		{
			if (ui.tedMood->isVisible())
				finishEditMood();
		}
	}
	return QWidget::eventFilter(AObject, AEvent);
}

void StatusWidget::onAddAvatarTriggered()
{
#ifdef Q_WS_WIN
	NonModalOpenFileDialog * fd = new NonModalOpenFileDialog;
	connect(fd, SIGNAL(fileNameSelected(const QString &)), SLOT(onAvatarFileSelected(const QString &)));
	fd->show(tr("Select new avatar image"), tr("Image files %1").arg("(*.jpg *.bmp *.png)") + "|*.jpg;*.bmp;*.png|");
#else
	QFileDialog * dialog = new QFileDialog;
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);
	dialog->setWindowTitle(tr("Select new avatar image"));
	dialog->setNameFilter(tr("Image files %1").arg("(*.jpg *.bmp *.png)"));
	connect(dialog, SIGNAL(fileSelected(const QString &)), SLOT(onAvatarFileSelected(const QString &)));
	dialog->show();
#endif
}

void StatusWidget::onAvatarFileSelected(const QString & fileName)
{
	if (!fileName.isEmpty())
	{
		QImage avatar;
		if (avatar.load(fileName))
			FAvatars->setAvatar(FStreamJid,avatar);
	}
}

void StatusWidget::onManageProfileTriggered()
{
	QDesktopServices::openUrl(QUrl("http://id.rambler.ru/"));
}

void StatusWidget::onProfileMenuAboutToHide()
{
	FAvatarHovered = false;
	ui.lblAvatar->repaint();
}

void StatusWidget::onProfileMenuAboutToShow()
{
	FAvatarHovered = true;
	ui.lblAvatar->repaint();
}

void StatusWidget::onVCardReceived(const Jid &AContactJid)
{
	if (FStreamJid && AContactJid)
	{
		IVCard *vcard = FVCardPlugin->vcard(AContactJid);
		if (vcard)
		{
			QString name = vcard->value(VVN_NICKNAME);
			if (name.isEmpty())
				name = vcard->value(VVN_FULL_NAME);
			if (name.isEmpty())
				name = vcard->value(VVN_GIVEN_NAME);
			if (name.isEmpty())
				name = vcard->contactJid().node();
			setUserName(name);
			vcard->unlock();
		}
	}
}

void StatusWidget::onStatusChanged(const Jid &AStreamJid, int AStatusId)
{
	if (AStreamJid == FStreamJid)
	{
		setMoodText(FStatusChanger->statusItemText(AStatusId));
		if (FAvatars)
			FAvatars->insertAutoAvatar(ui.lblAvatar, FStreamJid, QSize(24, 24), "pixmap");
	}
}
