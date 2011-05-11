#include "customlabel.h"
#include <QTextDocument>
#include <QPainter>
#include <QStyleOption>

CustomLabel::CustomLabel(QWidget *parent) :
	QLabel(parent)
{
}

void CustomLabel::paintEvent(QPaintEvent * pe)
{
	if ((!text().isEmpty()) &&
			(textFormat() == Qt::PlainText ||
			 (textFormat() == Qt::AutoText && Qt::mightBeRichText(text()))))
	{
		QPainter painter(this);
		QRectF lr = contentsRect();
		QStyleOption opt;
		opt.initFrom(this);
		int align = QStyle::visualAlignment(text().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight, alignment());
		int flags = align | (!text().isRightToLeft() ? Qt::TextForceLeftToRight : Qt::TextForceRightToLeft);
		style()->drawItemText(&painter, lr.toRect(), flags, opt.palette, isEnabled(), text(), foregroundRole());
	}
	else
		QLabel::paintEvent(pe);
}
