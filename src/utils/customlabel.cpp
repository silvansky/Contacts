#include "customlabel.h"
#include <QTextDocument>
#include <QPainter>
#include <QStyleOption>
#include <definitions/textflags.h>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

CustomLabel::CustomLabel(QWidget *parent) :
	QLabel(parent)
{
	shadowType = DarkShadow;
}

int CustomLabel::shadow() const
{
	return shadowType;
}

void CustomLabel::setShadow(int shadow)
{
	shadowType = (ShadowType)shadow;
#ifdef DEBUG_ENABLED
	qDebug() << "CustomLabel::setShadow: " << shadow;
#endif
}

void CustomLabel::paintEvent(QPaintEvent * pe)
{
	if ((!text().isEmpty()) &&
			(textFormat() == Qt::PlainText ||
			 (textFormat() == Qt::AutoText && !Qt::mightBeRichText(text()))))
	{
		QPainter painter(this);
		QRectF lr = contentsRect();
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
		style()->drawItemText(&painter, lr.toRect(), flags, opt.palette, isEnabled(), text(), QPalette::WindowText);
	}
	else
		QLabel::paintEvent(pe);
}
