﻿#include "balloontip.h"

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>
#include <QBitmap>
#include <QPainter>
#include <QMouseEvent>

static BalloonTip *theSolitaryBalloonTip = NULL;

bool BalloonTip::isBalloonVisible()
{
	return theSolitaryBalloonTip;
}

QWidget *BalloonTip::showBalloon(QIcon icon, const QString& title, const QString& message,
				 const QPoint& pos, int timeout, bool showArrow, ArrowPosition arrowPosition)
{
	BalloonTip::hideBalloon();
	if (!(message.isEmpty() && title.isEmpty()))
	{
		theSolitaryBalloonTip = new BalloonTip(icon, title, message);
		theSolitaryBalloonTip->drawBalloon(pos, timeout, showArrow, arrowPosition);
	}
	return theSolitaryBalloonTip;
}

QWidget *BalloonTip::showBalloon(QIcon icon, QWidget * messageWidget,
	const QPoint& pos, int timeout, bool showArrow, ArrowPosition arrowPosition)
{
	BalloonTip::hideBalloon();
	if (messageWidget)
	{
		theSolitaryBalloonTip = new BalloonTip(icon, messageWidget);
		theSolitaryBalloonTip->drawBalloon(pos, timeout, showArrow, arrowPosition);
	}
	return theSolitaryBalloonTip;
}

void BalloonTip::hideBalloon()
{
	if (theSolitaryBalloonTip)
	{
		theSolitaryBalloonTip->close();
		delete theSolitaryBalloonTip;
		theSolitaryBalloonTip = NULL;
	}
}

void BalloonTip::init()
{
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setMaximumWidth(250);
	QPalette pal = palette();
	pal.setColor(QPalette::Window, pal.toolTipBase().color());
	pal.setColor(QPalette::WindowText, pal.toolTipText().color());
	setPalette(pal);
	widget = NULL;
}

BalloonTip::BalloonTip(QIcon icon, const QString& title, const QString& message) : QWidget(0, Qt::ToolTip), timerId(-1)
{
	init();

	QLabel *titleLabel = new QLabel;
	titleLabel->installEventFilter(this);
	titleLabel->setText(title);
	QFont font = titleLabel->font();
	font.setBold(true);
#ifdef Q_WS_WINCE
	font.setPointSize(font.pointSize() - 2);
#endif
	titleLabel->setFont(font);
	titleLabel->setTextFormat(Qt::PlainText); // to maintain compat with windows

#ifdef Q_WS_WINCE
	const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
#else
	const int iconSize = 18;
#endif

	QLabel *msgLabel = new QLabel;
#ifdef Q_WS_WINCE
	font.setBold(false);
	msgLabel->setFont(font);
#endif
	msgLabel->installEventFilter(this);
	msgLabel->setText(message);
	msgLabel->setTextFormat(Qt::PlainText);
	msgLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	// smart size for the message label
#ifdef Q_WS_WINCE
	int limit = QApplication::desktop()->availableGeometry(msgLabel).size().width() / 2;
#else
	int limit = 200; // QApplication::desktop()->availableGeometry(msgLabel).size().width() / 4;
#endif
	if (msgLabel->sizeHint().width() > limit)
	{
		msgLabel->setWordWrap(true);
#ifdef Q_WS_WINCE
		// Make sure that the text isn't wrapped "somewhere" in the balloon widget
		// in the case that we have a long title label.
		setMaximumWidth(limit);
#else
		// Here we allow the text being much smaller than the balloon widget
		// to emulate the weird standard windows behavior.
		msgLabel->setFixedSize(limit, msgLabel->heightForWidth(limit));
#endif
	}

	QGridLayout *layout = new QGridLayout;
	if (!icon.isNull())
	{
		QLabel *iconLabel = new QLabel;
		iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
		iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		iconLabel->setMargin(2);
		layout->addWidget(iconLabel, 0, 0);
		if (!title.isEmpty())
			layout->addWidget(titleLabel, 0, 1);
	}
	else
	{
		if (!title.isEmpty())
			layout->addWidget(titleLabel, 0, 0, 1, 2);
	}

	if (title.isEmpty())
	{
		layout->addWidget(msgLabel, 0, 1, 2, 1);
		layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0);
	}
	else
		layout->addWidget(msgLabel, 1, 0, 1, 3);
	layout->setSizeConstraint(QLayout::SetFixedSize);
	layout->setMargin(3);
	setLayout(layout);
}

BalloonTip::BalloonTip(QIcon icon, QWidget * messageWidget) : QWidget(0, Qt::ToolTip), timerId(-1)
{
	init();
	widget = messageWidget;
	QIcon si = icon;
	QGridLayout *layout = new QGridLayout;
#ifdef Q_WS_WINCE
	const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
#else
	const int iconSize = 18;
#endif
	if (!si.isNull())
	{
		QLabel *iconLabel = new QLabel;
		iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		iconLabel->setPixmap(si.pixmap(iconSize, iconSize));
		iconLabel->setMargin(2);
		layout->addWidget(iconLabel, 0, 0);
		layout->addWidget(widget, 0, 1);
	}
	else
	{
		layout->addWidget(widget, 0, 0, 1, 2);
	}
	layout->setSizeConstraint(QLayout::SetFixedSize);
	layout->setMargin(3);
	setLayout(layout);
}

BalloonTip::~BalloonTip()
{
	theSolitaryBalloonTip = NULL;
	if (widget)
	{
		widget->setParent(0);
		widget = NULL;
	}
	emit closed();
}

void BalloonTip::drawBalloon(const QPoint& pos, int msecs, bool showArrow, ArrowPosition arrowPosition)
{
	QRect scr = QApplication::desktop()->screenGeometry(pos);
	QSize sh = sizeHint();
	const int border = 0;
	int ah = 18, ao = 18, aw = 18, rc = 7;
	if (arrowPosition == ArrowLeft || arrowPosition == ArrowRight)
	{
		ah = 10, ao = 14, aw = 5;
	}
	// determining arrow position (only top and bottom are auto)
	if (arrowPosition == AutoArrow)
	{
		if (pos.y() + sh.height() + ah < scr.height())
			arrowPosition = ArrowTop;
		else
			arrowPosition = ArrowBottom;
	}
	// vertical alignment for left and right arrow position
	bool arrowAtTop = (pos.y() + sh.height() + ah < scr.height());
	// horisontal alignment for top and bottom arrow position
	bool arrowAtLeft = (pos.x() + sh.width() - ao < scr.width());
	setContentsMargins(border + (arrowPosition == ArrowLeft ? aw : 0) + 3,  border + (arrowPosition == ArrowTop ? ah : 0) + 2, border + (arrowPosition == ArrowRight ? aw : 0) + 3, border + (arrowPosition == ArrowBottom ? ah : 0) + 2);
	updateGeometry();
	sh  = sizeHint();

	int ml, mr, mt, mb;
	QSize sz = sizeHint();
	switch (arrowPosition)
	{
	case ArrowLeft:
		ml = aw;
		mt = 0;
		mr = sz.width() - 1;
		mb = sz.height() - 1;
		break;
	case ArrowRight:
		ml = mt = 0;
		mr = sz.width() - aw - 1;
		mb = sz.height() - 1;
		break;
	case ArrowTop:
		ml = 0;
		mt = ah;
		mr = sz.width() - 1;
		mb = sz.height() - 1;
		break;
	case ArrowBottom:
		ml = mt = 0;
		mr = sz.width() - 1;
		mb = sz.height() - ah - 1;
		break;
	default:
		break;
	}

	QPainterPath path;
#if defined(QT_NO_XSHAPE) && defined(Q_WS_X11)
	// XShape is required for setting the mask, so we just
	// draw an ugly square when its not available
	path.moveTo(0, 0);
	path.lineTo(sz.width() - 1, 0);
	path.lineTo(sz.width() - 1, sz.height() - 1);
	path.lineTo(0, sz.height() - 1);
	path.lineTo(0, 0);
	move(qMax(pos.x() - sz.width(), scr.left()), pos.y());
#else
	path.moveTo(ml + rc, mt);
	if ((arrowPosition == ArrowTop) && arrowAtLeft)
	{
		if (showArrow)
		{
			path.lineTo(ml + ao, mt);
			path.lineTo(ml + ao, mt - ah);
			path.lineTo(ml + ao + aw, mt);
		}
		move(qMax(pos.x() - ao, scr.left() + 2), pos.y());
	}
	else if ((arrowPosition == ArrowTop) && !arrowAtLeft)
	{
		if (showArrow)
		{
			path.lineTo(mr - ao - aw, mt);
			path.lineTo(mr - ao, mt - ah);
			path.lineTo(mr - ao, mt);
		}
		move(qMin(pos.x() - sh.width() + ao, scr.right() - sh.width() - 2), pos.y());
	}
	path.lineTo(mr - rc, mt);
	path.arcTo(QRect(mr - rc*2, mt, rc*2, rc*2), 90, -90);
	if ((arrowPosition == ArrowRight) && arrowAtTop)
	{
		if (showArrow)
		{
			path.lineTo(mr, mt + rc + ao);
			path.lineTo(mr + aw, mt + rc + ao + ah/2);
			path.lineTo(mr, mt + rc + ao + ah);
		}
		move(pos.x() - sh.width(), pos.y() - ao - rc - ah/2);
	}
	else if ((arrowPosition == ArrowRight) && !arrowAtTop)
	{
		if (showArrow)
		{
			path.lineTo(mr, mb - rc - ao - ah);
			path.lineTo(mr + aw, mb - rc - ao - ah/2);
			path.lineTo(mr, mb - rc - ao);
		}
		move(pos.x() - sh.width(), pos.y() - (sh.height() - ao - rc - ah/2));
	}
	path.lineTo(mr, mb - rc);
	path.arcTo(QRect(mr - rc*2, mb - rc*2, rc*2, rc*2), 0, -90);
	if ((arrowPosition == ArrowBottom) && !arrowAtLeft)
	{
		if (showArrow)
		{
			path.lineTo(mr - ao, mb);
			path.lineTo(mr - ao, mb + ah);
			path.lineTo(mr - ao - aw, mb);
		}
		move(qMin(pos.x() - sh.width() + ao, scr.right() - sh.width() - 2),
		     pos.y() - sh.height());
	}
	else if ((arrowPosition == ArrowBottom) && arrowAtLeft)
	{
		if (showArrow)
		{
			path.lineTo(ao + aw, mb);
			path.lineTo(ao, mb + ah);
			path.lineTo(ao, mb);
		}
		move(qMax(pos.x() - ao, scr.x() + 2), pos.y() - sh.height());
	}
	path.lineTo(ml + rc, mb);
	path.arcTo(QRect(ml, mb - rc*2, rc*2, rc*2), -90, -90);
	if ((arrowPosition == ArrowLeft) && arrowAtTop)
	{
		if (showArrow)
		{
			path.lineTo(ml, mt + rc + ao + ah);
			path.lineTo(ml - aw, mt + rc + ao + ah/2);
			path.lineTo(ml, mt + rc + ao);
		}
		move(pos.x(), pos.y() - ao - rc - ah/2);
	}
	else if ((arrowPosition == ArrowLeft) && !arrowAtTop)
	{
		if (showArrow)
		{
			path.lineTo(ml, mb - rc - ao);
			path.lineTo(ml - aw, mb - rc - ao - ah/2);
			path.lineTo(ml, mb - rc - ao - ah);
		}
		move(pos.x(), pos.y() - (sh.height() - ao - rc - ah/2));
	}
	path.lineTo(ml, mt + rc);
	path.arcTo(QRect(ml, mt, rc*2, rc*2), 180, -90);

	// Set the mask
	QBitmap bitmap = QBitmap(sizeHint());
	bitmap.fill(Qt::color0);
	QPainter painter1(&bitmap);
	painter1.setPen(QPen(Qt::color1, border));
	painter1.setBrush(QBrush(Qt::color1));
	painter1.drawPath(path);
	setMask(bitmap);
#endif

	// Draw the border
	pixmap = QPixmap(sz);
	pixmap.fill(QColor(0, 0, 0, 0));
	QPainter painter2(&pixmap);
	painter2.setPen(QPen(palette().color(QPalette::Window).darker(160), border));
	painter2.setBrush(palette().color(QPalette::Window));
	painter2.drawPath(path);

	if (msecs > 0)
		timerId = startTimer(msecs);
	show();
}

void BalloonTip::paintEvent(QPaintEvent *evt)
{
	QPainter painter(this);
	painter.drawPixmap(rect(), pixmap);
	QWidget::paintEvent(evt);
}

void BalloonTip::mousePressEvent(QMouseEvent *ev)
{
	if(ev->button() == Qt::LeftButton)
		emit messageClicked();
	deleteLater();
	QWidget::mousePressEvent(ev);
}

void BalloonTip::timerEvent(QTimerEvent *ev)
{
	if (ev->timerId() == timerId)
	{
		killTimer(timerId);
		if (!underMouse())
			deleteLater();
	}
	QWidget::timerEvent(ev);
}
