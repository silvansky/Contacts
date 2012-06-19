#include "customlabel.h"
#include "imagemanager.h"
#include "graphicseffectsstorage.h"

#include <QPainter>
#include <QStyleOption>
#include <QTextLayout>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#include <definitions/textflags.h>
#include <definitions/graphicseffects.h>
#include <definitions/resources.h>

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

QSize CustomLabel::sizeHint() const
{
	QSize sh = QLabel::sizeHint();
	//sh.setWidth(sh.width() + 1);
	//sh.setHeight(sh.height() + 4);
#ifdef DEBUG_ENABLED
	qDebug() << "for text:" << text();
	qDebug() << "  size hint:" << sh;
#endif
	QTextDocument *doc = textDocument();
	sh = doc->documentLayout()->documentSize().toSize();
#ifdef DEBUG_ENABLED
	qDebug() << "  doc size:" << sh;
#endif
	sh += QSize(doc->documentMargin(), doc->documentMargin());
#ifdef DEBUG_ENABLED
	qDebug() << "  doc size with margin:" << sh;
#endif
	return sh;
}

void CustomLabel::paintEvent(QPaintEvent *pe)
{
	if ((!text().isEmpty()) &&
			(textFormat() == Qt::PlainText ||
			 (textFormat() == Qt::AutoText && !Qt::mightBeRichText(text()))))
	{
		QPainter painter(this);
//		int align = QStyle::visualAlignment(text().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight, alignment());
//		int flags = align | (!text().isRightToLeft() ? Qt::TextForceLeftToRight : Qt::TextForceRightToLeft);
//		if (wordWrap())
//			flags |= Qt::TextWordWrap;
//		switch (shadowType)
//		{
//		case NoShadow:
//			flags |= TF_NOSHADOW;
//			break;
//		case DarkShadow:
//			flags |= TF_DARKSHADOW;
//			break;
//		case LightShadow:
//			flags |= TF_LIGHTSHADOW;
//			break;
//		default:
//			break;
//		}
		QTextDocument *doc = textDocument();
		QAbstractTextDocumentLayout::PaintContext ctx = textDocumentPaintContext(doc);
		QString shadowKey;
		switch (shadowType)
		{
		case DarkShadow:
			shadowKey = GFX_TEXTSHADOWS;
			break;
		case LightShadow:
			shadowKey = GFX_NOTICEWIDGET;
			break;
		case NoShadow:
		default:
			break;
		}

		// magic numbers
		int dx = -2;
		int dy = -6;
		// adding margins
		dx += contentsMargins().left();
		dy += contentsMargins().top();

#if 1 // for debug set 0
		QGraphicsDropShadowEffect *shadow = qobject_cast<QGraphicsDropShadowEffect *>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(shadowKey));
#else
		QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
		shadow->setColor(Qt::red);
		shadow->setOffset(20, 20);
#endif
		if (shadow)
		{
			QImage shadowedText(size(), QImage::Format_ARGB32_Premultiplied);
#if defined(Q_WS_MAC) && !defined(__MAC_OS_X_NATIVE_FULLSCREEN)
			// TODO: fix that
			shadowedText.fill(Qt::red); // DUNNO WHY!!!
#else
			shadowedText.fill(Qt::transparent);
#endif
			QPainter tmpPainter(&shadowedText);
			tmpPainter.setRenderHint(QPainter::Antialiasing);
			tmpPainter.setRenderHint(QPainter::HighQualityAntialiasing);
			tmpPainter.setRenderHint(QPainter::TextAntialiasing);
			tmpPainter.setRenderHint(QPainter::SmoothPixmapTransform);
			tmpPainter.translate(dx, dy);
			doc->documentLayout()->draw(&tmpPainter, ctx);
			painter.drawImage(0, 0, shadowedText);
		}
		else
		{
			painter.save();
			painter.translate(dx, dy);
			doc->documentLayout()->draw(&painter, ctx);
			painter.restore();
		}
		doc->deleteLater();
	}
	else
		QLabel::paintEvent(pe);
}

QString CustomLabel::elidedText() const
{
	QString elided = text();
	QStringList srcLines;
	QStringList dstLines;
	QRectF lr = contentsRect();
	int textWidth = lr.width();
	// eliding text
	// TODO: move to text change / resize event handler, make elided a member
	if (elideMode() != Qt::ElideNone)
	{
		QFontMetrics fm = fontMetrics();
		if (!wordWrap())
		{
			// single line elide
			elided = fm.elidedText(text(), elideMode(), textWidth);
		}
		else if (elideMode() == Qt::ElideRight)
		{
			// multiline elide
			srcLines = elided.split("\n");
			int pxPerLine = fm.lineSpacing();
			int lines = lr.height() / pxPerLine + 1;

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
			elided = dstLines.join("\r\n");
		}
	}
	return elided;
}

QTextDocument *CustomLabel::textDocument() const
{
	QString textToDraw = elidedText();
	QTextDocument *doc = new QTextDocument;
	doc->setDefaultFont(font());
	doc->setPlainText(textToDraw);
	doc->setTextWidth(wordWrap() ? contentsRect().width() - 1 : -1);
	//doc->setPageSize(QSizeF(wordWrap() ? contentsRect().width() - 1 : -1, contentsRect().height() - 1));
	QTextOption textOpt(alignment());
	textOpt.setWrapMode(wordWrap() ? QTextOption::WordWrap : QTextOption::NoWrap);
	textOpt.setTextDirection(layoutDirection());
	doc->setDefaultTextOption(textOpt);
	return doc;
}

QAbstractTextDocumentLayout::PaintContext CustomLabel::textDocumentPaintContext(QTextDocument *doc) const
{
	QStyleOption opt;
	opt.initFrom(this);
	QAbstractTextDocumentLayout::PaintContext ctx;
	ctx.cursorPosition = -1;
	ctx.palette = opt.palette;
	ctx.clip = QRectF(contentsMargins().left(), contentsMargins().top(), contentsRect().width(), contentsRect().height());
	if (hasSelectedText())
	{
		QAbstractTextDocumentLayout::Selection sel;
		QTextCursor cur(doc);
		cur.setPosition(selectionStart());
		cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, selectedText().length());
		sel.cursor = cur;
		QTextCharFormat fmt;
		fmt.setBackground(opt.palette.highlight());
		fmt.setForeground(opt.palette.highlightedText());
		sel.format = fmt;
		ctx.selections << sel;
	}
	return ctx;
}
