#include "qhtmltoolbutton.h"
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QPainter>
#include <QApplication>

// piece of code from qt\src\gui\styles\qcommonstyle.cpp
static void drawArrow(const QStyle *style, const QStyleOptionToolButton *toolbutton,
		      const QRect &rect, QPainter *painter, const QWidget *widget = 0)
{
	QStyle::PrimitiveElement pe;
	switch (toolbutton->arrowType)
	{
	case Qt::LeftArrow:
		pe = QStyle::PE_IndicatorArrowLeft;
		break;
	case Qt::RightArrow:
		pe = QStyle::PE_IndicatorArrowRight;
		break;
	case Qt::UpArrow:
		pe = QStyle::PE_IndicatorArrowUp;
		break;
	case Qt::DownArrow:
		pe = QStyle::PE_IndicatorArrowDown;
		break;
	default:
		return;
	}
	QStyleOption arrowOpt;
	arrowOpt.rect = rect;
	arrowOpt.palette = toolbutton->palette;
	arrowOpt.state = toolbutton->state;
	style->drawPrimitive(pe, &arrowOpt, painter, widget);
}

QHtmlToolButton::QHtmlToolButton(QWidget *parent) :
		QToolButton(parent)
{
}

QString QHtmlToolButton::html() const
{
	return text();
}

QSize QHtmlToolButton::sizeHint() const
{
	// code based on qt\src\gui\widgets\qtoolbutton.cpp : L 422 - 458
	ensurePolished();
	int w = 0, h = 0;
	QStyleOptionToolButton opt;
	initStyleOption(&opt);

	QFontMetrics fm = fontMetrics();
	if (opt.toolButtonStyle != Qt::ToolButtonTextOnly)
	{
		QSize icon = opt.iconSize;
		w = icon.width();
		h = icon.height();
	}

	if (opt.toolButtonStyle != Qt::ToolButtonIconOnly)
	{
		QTextDocument doc(text());
		QSize textSize = QSize(int(doc.size().width()), int(doc.size().height()));
		textSize.setWidth(textSize.width() + fm.width(QLatin1Char(' '))*2);
		if (opt.toolButtonStyle == Qt::ToolButtonTextUnderIcon)
		{
			h += 4 + textSize.height();
			if (textSize.width() > w)
				w = textSize.width();
		}
		else if (opt.toolButtonStyle == Qt::ToolButtonTextBesideIcon)
		{
			w += 4 + textSize.width();
			if (textSize.height() > h)
				h = textSize.height();
		}
		else
		{ // TextOnly
			w = textSize.width();
			h = textSize.height();
		}
	}

	opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
	if (popupMode() == MenuButtonPopup)
		w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

	return style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut());
}

void QHtmlToolButton::paintEvent(QPaintEvent *)
{
	QStyleOptionToolButton *opt = new QStyleOptionToolButton;
	initStyleOption(opt);
	QPainter * p = new QPainter(this);
	// code based on qt\src\gui\styles\qcommonstyle.cpp : L 3326-3384
	QRect button, menuarea;
	button = style()->proxy()->subControlRect(QStyle::CC_ToolButton, opt, QStyle::SC_ToolButton, this);
	menuarea = style()->proxy()->subControlRect(QStyle::CC_ToolButton, opt, QStyle::SC_ToolButtonMenu, this);

	QStyle::State bflags = opt->state & ~QStyle::State_Sunken;

	if (bflags & QStyle::State_AutoRaise)
	{
		if (!(bflags & QStyle::State_MouseOver) || !(bflags & QStyle::State_Enabled))
		{
			bflags &= ~QStyle::State_Raised;
		}
	}
	QStyle::State mflags = bflags;
	if (opt->state & QStyle::State_Sunken)
	{
		if (opt->activeSubControls & QStyle::SC_ToolButton)
			bflags |= QStyle::State_Sunken;
		mflags |= QStyle::State_Sunken;
	}

	QStyleOption tool(0);
	tool.palette = opt->palette;
	if (opt->subControls & QStyle::SC_ToolButton)
	{
		if (bflags & (QStyle::State_Sunken | QStyle::State_On | QStyle::State_Raised))
		{
			tool.rect = button;
			tool.state = bflags;
			style()->proxy()->drawPrimitive(QStyle::PE_PanelButtonTool, &tool, p, this);
		}
	}

	if (opt->state & QStyle::State_HasFocus)
	{
		QStyleOptionFocusRect fr;
		fr.QStyleOption::operator=(*opt);
		fr.rect.adjust(3, 3, -3, -3);
		if (opt->features & QStyleOptionToolButton::MenuButtonPopup)
			fr.rect.adjust(0, 0, -style()->proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator, opt, this), 0);
		style()->proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &fr, p, this);
	}
	QStyleOptionToolButton label = *opt;
	label.state = bflags;
	int fw = style()->proxy()->pixelMetric(QStyle::PM_DefaultFrameWidth, opt, this);
	label.rect = button.adjusted(fw, fw, -fw, -fw);
	// here is the drawing of the label
	// the original version:
	// style()->proxy()->drawControl(QStyle::CE_ToolButtonLabel, &label, p, this);
	// new version:
	QTextDocument doc;
	doc.setHtml(html());
	// based on qt\src\gui\styles\qcommonstyle.cpp
	QRect rect = label.rect;
	int shiftX = 0;
	int shiftY = 0;
	if (label.state & (QStyle::State_Sunken | QStyle::State_On))
	{
		shiftX = style()->proxy()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &label, this);
		shiftY = style()->proxy()->pixelMetric(QStyle::PM_ButtonShiftVertical, &label, this);
	}
	// Arrow type always overrules and is always shown
	bool hasArrow = label.features & QStyleOptionToolButton::Arrow;
	if (((!hasArrow && label.icon.isNull()) && !label.text.isEmpty()) || label.toolButtonStyle == Qt::ToolButtonTextOnly)
	{
		int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
		if (!style()->proxy()->styleHint(QStyle::SH_UnderlineShortcut, &label, this))
			alignment |= Qt::TextHideMnemonic;
		rect.translate(shiftX, shiftY);
		p->setFont(label.font);
		// !!!!!!!!!!!!!
//		style()->proxy()->drawItemText(p, rect, alignment, label.palette,
//					       opt->state & QStyle::State_Enabled, label.text,
//					       QPalette::ButtonText);
		doc.drawContents(p, rect);
	}
	else
	{
		QPixmap pm;
		QSize pmSize = label.iconSize;
		if (!label.icon.isNull())
		{
			QIcon::State state = label.state & QStyle::State_On ? QIcon::On : QIcon::Off;
			QIcon::Mode mode;
			if (!(label.state & QStyle::State_Enabled))
				mode = QIcon::Disabled;
			else if ((label.state & QStyle::State_MouseOver) && (label.state & QStyle::State_AutoRaise))
				mode = QIcon::Active;
			else
				mode = QIcon::Normal;
			pm = label.icon.pixmap(label.rect.size().boundedTo(label.iconSize),
					       mode, state);
			pmSize = pm.size();
		}

		if (label.toolButtonStyle != Qt::ToolButtonIconOnly)
		{
			p->setFont(label.font);
			QRect pr = rect,
			tr = rect;
			int alignment = Qt::TextShowMnemonic;
			if (!style()->proxy()->styleHint(QStyle::SH_UnderlineShortcut, &label, this))
				alignment |= Qt::TextHideMnemonic;

			if (label.toolButtonStyle == Qt::ToolButtonTextUnderIcon)
			{
				pr.setHeight(pmSize.height() + 6);
				tr.adjust(0, pr.height() - 1, 0, -3);
				pr.translate(shiftX, shiftY);
				if (!hasArrow)
				{
					style()->proxy()->drawItemPixmap(p, pr, Qt::AlignCenter, pm);
				}
				else
				{
					drawArrow(style(), &label, pr, p, this);
				}
				alignment |= Qt::AlignCenter;
			}
			else
			{
				pr.setWidth(pmSize.width() + 8);
				tr.adjust(pr.width(), 0, 0, 0);
				pr.translate(shiftX, shiftY);
				if (!hasArrow)
				{
					style()->proxy()->drawItemPixmap(p, QStyle::visualRect(label.direction, rect, pr), Qt::AlignCenter, pm);
				}
				else
				{
					drawArrow(style(), &label, pr, p, this);
				}
				alignment |= Qt::AlignLeft | Qt::AlignVCenter;
			}
			tr.translate(shiftX, shiftY);
			// !!!!!!!!!!!!!!
			tr = QStyle::visualRect(opt->direction, rect, tr);
			p->save();
			p->translate(tr.x(), -1);
			doc.drawContents(p, rect);
			p->restore();
//			style()->proxy()->drawItemText(p, QStyle::visualRect(opt->direction, rect, tr), alignment, label.palette,
//						       label.state & QStyle::State_Enabled, label.text,
//						       QPalette::ButtonText);
		}
		else
		{
			rect.translate(shiftX, shiftY);
			if (hasArrow)
			{
				drawArrow(style(), &label, rect, p, this);
			}
			else
			{
				style()->proxy()->drawItemPixmap(p, rect, Qt::AlignCenter, pm);
			}
		}
	}

	// end of label drawing

	if (opt->subControls & QStyle::SC_ToolButtonMenu)
	{
		tool.rect = menuarea;
		tool.state = mflags;
		if (mflags & (QStyle::State_Sunken | QStyle::State_On | QStyle::State_Raised))
			style()->proxy()->drawPrimitive(QStyle::PE_IndicatorButtonDropDown, &tool, p, this);
		style()->proxy()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &tool, p, this);
	}
	else if (opt->features & QStyleOptionToolButton::HasMenu)
	{
		int mbi = style()->proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator, opt, this);
		QRect ir = opt->rect;
		QStyleOptionToolButton newBtn = *opt;
		newBtn.rect = QRect(ir.right() + 5 - mbi, ir.y() + ir.height() - mbi + 4, mbi - 6, mbi - 6);
		style()->proxy()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &newBtn, p, this);
	}
	p->end();
	delete p;
}

void QHtmlToolButton::setHtml(const QString & html)
{
	setText(html);
	emit htmlChanged(html);
}
