#include "customlabel.h"
#include <QTextDocument>
#include <QPainter>
#include <QStyleOption>
#include <definitions/textflags.h>

CustomLabel::CustomLabel(QWidget *parent) :
	QLabel(parent)
{
	shadowType = DarkShadow;
	textElideMode = Qt::ElideNone;
}

int CustomLabel::shadow() const
{
	return shadowType;
}

void CustomLabel::setShadow(int shadow)
{
	shadowType = (ShadowType)shadow;
	update();
}

Qt::TextElideMode CustomLabel::elideMode() const
{
	return textElideMode;
}

void CustomLabel::setElideMode(/*Qt::TextElideMode*/ int mode)
{
	textElideMode = (Qt::TextElideMode)mode;
	update();
}

bool CustomLabel::multilineElideEnabled() const
{
	return multilineElide;
}

void CustomLabel::setMultilineElideEnabled(bool on)
{
	multilineElide = on;
	update();
}

void CustomLabel::paintEvent(QPaintEvent * pe)
{
	if ((!text().isEmpty()) &&
			(textFormat() == Qt::PlainText ||
			 (textFormat() == Qt::AutoText && !Qt::mightBeRichText(text()))))
	{
		QPainter painter(this);
		QRectF lr = contentsRect();
		lr.moveBottom(lr.bottom() - 1); // angry and dirty hack!
		QStyleOption opt;
		opt.initFrom(this);
		int align = QStyle::visualAlignment(text().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight, alignment());
		int flags = align | (!text().isRightToLeft() ? Qt::TextForceLeftToRight : Qt::TextForceRightToLeft);
		if (wordWrap())
			flags |= Qt::TextWordWrap;
		switch (shadowType)
		{
		case NoShadow:
			flags |= TF_NOSHADOW;
			break;
		case DarkShadow:
			flags |= TF_DARKSHADOW;
			break;
		case LightShadow:
			flags |= TF_LIGHTSHADOW;
			break;
		default:
			break;
		}
		QString textToDraw = text();
		int wd = lr.width();
		if ((elideMode() != Qt::ElideNone) && !wordWrap())
		{
			textToDraw = fontMetrics().elidedText(text(), elideMode(), wd);
		}
		style()->drawItemText(&painter, lr.toRect(), flags, opt.palette, isEnabled(), textToDraw, QPalette::WindowText);
	}
	else
		QLabel::paintEvent(pe);
}
