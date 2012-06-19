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
	multilineElide = false;
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
		int textWidth = lr.width();
		// eliding text
		// TODO: move to text change / resize event handler, make textToDraw a member
		if (elideMode() != Qt::ElideNone)
		{
			QFontMetrics fm = fontMetrics();
			if (!wordWrap())
			{
				textToDraw = fm.elidedText(text(), elideMode(), textWidth);
			}
			else if (elideMode() == Qt::ElideRight)
			{
				// multiline elide
				int pxPerLine = fontMetrics().lineSpacing();
				int lines = lr.height() / pxPerLine + 1;

				QStringList srcLines = text().split("\n");
				QStringList dstLines;
				foreach (QString srcLine, srcLines)
				{
					int w = fm.width(srcLine);
					if (w >= textWidth)
					{
						QStringList tmpList = srcLine.split(' ');
						QString s;
						int i = 0;
						while (i < tmpList.count())
						{
							if (fm.width(s + " " + tmpList.at(i)) >= textWidth)
							{
								if (!s.isEmpty())
								{
									dstLines += s;
									s = QString::null;
								}
							}
							if (!s.isEmpty())
							{
								s += " ";
							}
							s += tmpList.at(i);
							i++;
						}
						dstLines += s;
					}
					else
					{
						dstLines += srcLine;
					}
				}
				int n = dstLines.count();
				dstLines = dstLines.mid(0, lines);
				if (n > lines)
				{
					dstLines.last() += "...";
				}
				for (QStringList::iterator it = dstLines.begin(); it != dstLines.end(); it++)
				{
					*it = fm.elidedText(*it, elideMode(), textWidth);
				}
				textToDraw = dstLines.join("\r\n");
			}
		}
		style()->drawItemText(&painter, lr.toRect(), flags, opt.palette, isEnabled(), textToDraw, QPalette::WindowText);
	}
	else
		QLabel::paintEvent(pe);
}
