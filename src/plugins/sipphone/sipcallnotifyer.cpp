#include "sipcallnotifyer.h"
#include "ui_sipcallnotifyer.h"

#include <QPropertyAnimation>
#include <QDesktopWidget>

#include <utils/stylestorage.h>
#include <utils/iconstorage.h>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>

SipCallNotifyer::SipCallNotifyer(const QString & caption, const QString & notice, const QIcon & icon, const QImage & avatar) :
	QWidget(NULL),
	ui(new Ui::SipCallNotifyer)
{
	ui->setupUi(this);
	ui->caption->setText(caption);
	ui->notice->setText(notice);
	ui->icon->setPixmap(icon.pixmap(icon.availableSizes().at(0)));
	ui->avatar->setPixmap(QPixmap::fromImage(avatar));

	ui->accept->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_BTN_ACCEPT));
	ui->reject->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_BTN_HANGUP));

	connect(ui->accept, SIGNAL(clicked()), SLOT(acceptClicked()));
	connect(ui->reject, SIGNAL(clicked()), SLOT(rejectClicked()));
	connect(ui->mute, SIGNAL(clicked()), SLOT(muteClicked()));

	_muted = false;

	// style
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE_CALL_NOTIFYER);

	// border
	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_NOTIFICATION);
	if (border)
	{
		border->setResizable(false);
		border->setMovable(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(border, SIGNAL(closeClicked()), SLOT(rejectClicked()));
	}
}

SipCallNotifyer::~SipCallNotifyer()
{
	delete ui;
}

bool SipCallNotifyer::isMuted() const
{
	return _muted;
}

void SipCallNotifyer::appear()
{
	QPropertyAnimation * animation = new QPropertyAnimation;
	animation->setDuration(1000); // 1 sec
	animation->setEasingCurve(QEasingCurve(QEasingCurve::InQuad));
	animation->setStartValue(0.0);
	animation->setEndValue(1.0);
	connect(animation, SIGNAL(valueChanged(const QVariant&)), SLOT(animationValueChanged(const QVariant&)));

	QWidget * w = (border ? (QWidget *)border : (QWidget *)this);
	if (!w->isVisible())
	{
		w->setWindowOpacity(0.0);
		w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
		w->adjustSize();
		QRect wrect = w->geometry();
		QDesktopWidget * dw = QApplication::desktop();
		QRect screen = dw->screenGeometry(dw->primaryScreen());
		QRect centered = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, wrect.size(), screen);
		w->setGeometry(centered);
		w->show();
	}

	animation->start(QAbstractAnimation::DeleteWhenStopped);
	// TODO: start ringing sound
}

void SipCallNotifyer::disappear()
{
	(border ? (QWidget *)border : (QWidget *)this)->hide();
}

void SipCallNotifyer::acceptClicked()
{
	emit accepted();
	disappear();
}

void SipCallNotifyer::rejectClicked()
{
	emit rejected();
	disappear();
}

void SipCallNotifyer::muteClicked()
{
	_muted = !_muted;
	// TODO: mute sound
}

void SipCallNotifyer::animationValueChanged(const QVariant & value)
{
	(border ? (QWidget *)border : (QWidget *)this)->setWindowOpacity(value.toDouble());
}
