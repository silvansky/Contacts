#include "htmltoolbutton.h"

#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QPainter>
#include <QApplication>
//#include <definations/resources.h>
//#include <definations/menuicons.h>
//#include <utils/iconstorage.h>

// statics
//QImage HtmlToolButton::menuIndicatorUp;
//QImage HtmlToolButton::menuIndicatorDown;

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

HtmlToolButton::HtmlToolButton(QWidget *parent) :
		QToolButton(parent)
{
	//if (menuIndicatorUp.isNull())
	//{
	//	menuIndicatorUp.load(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_MENU_INDICATOR_UP));
	//}
	//if (menuIndicatorDown.isNull())
	//{
	//	menuIndicatorDown.load(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_MENU_INDICATOR_DOWN));
	//}
}

QString HtmlToolButton::html() const
{
	return text();
}

QSize HtmlToolButton::sizeHint() const
{
	// code based on qt\src\gui\widgets\qtoolbutton.cpp
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
		QTextDocument doc;
		doc.setHtml(text());
		QSize textSize = QSize(int(doc.size().width()), int(doc.size().height()));
		textSize.setWidth(textSize.width() + fm.width(QLatin1Char(' ')) * 2);
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
		{
			// TextOnly
			w = textSize.width();
			h = textSize.height();
		}
	}

	opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
	if (popupMode() == MenuButtonPopup)
		w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

	return style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut());
}

void HtmlToolButton::setHtml(const QString &AHtml)
{
	setText(AHtml);
}

void HtmlToolButton::paintEvent(QPaintEvent *)
{
	QStyleOptionToolButton *opt = new QStyleOptionToolButton;
	initStyleOption(opt);
	QPainter *painter = new QPainter(this);
#if QT_VERSION >= 0x040600
	const QStyle * paintStyle = style()->proxy();
#else
	const QStyle * paintStyle = style();
#endif
	// code based on qt\src\gui\styles\qcommonstyle.cpp
	QRect button, menuarea;
	button = paintStyle->subControlRect(QStyle::CC_ToolButton, opt, QStyle::SC_ToolButton, this);
	menuarea = paintStyle->subControlRect(QStyle::CC_ToolButton, opt, QStyle::SC_ToolButtonMenu, this);

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
			paintStyle->drawPrimitive(QStyle::PE_PanelButtonTool, &tool, painter, this);
		}
	}

	if (opt->state & QStyle::State_HasFocus)
	{
		QStyleOptionFocusRect fr;
		fr.QStyleOption::operator=(*opt);
		fr.rect.adjust(3, 3, -3, -3);
		if (opt->features & QStyleOptionToolButton::MenuButtonPopup)
			fr.rect.adjust(0, 0, -paintStyle->pixelMetric(QStyle::PM_MenuButtonIndicator, opt, this), 0);
		paintStyle->drawPrimitive(QStyle::PE_FrameFocusRect, &fr, painter, this);
	}
	QStyleOptionToolButton label = *opt;
	label.state = bflags;
	int fw = paintStyle->pixelMetric(QStyle::PM_DefaultFrameWidth, opt, this);
	label.rect = button.adjusted(fw, fw, -fw, -fw);

	// here is the drawing of the label
	// the original version:
	// paintStyle->drawControl(QStyle::CE_ToolButtonLabel, &label, painter, this);
	// new version:
	QTextDocument doc;
	doc.setHtml(html());
	// code based on qt\src\gui\styles\qcommonstyle.cpp
	QRect rect = label.rect;
	int shiftX = 0;
	int shiftY = 0;
	if (label.state & (QStyle::State_Sunken | QStyle::State_On))
	{
		shiftX = paintStyle->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &label, this);
		shiftY = paintStyle->pixelMetric(QStyle::PM_ButtonShiftVertical, &label, this);
	}
	if (isDown())
	{
		shiftX += 1;
		shiftY += 1;
	}
	// Arrow type always overrules and is always shown
	bool hasArrow = label.features & QStyleOptionToolButton::Arrow;
	if (((!hasArrow && label.icon.isNull()) && !label.text.isEmpty()) || label.toolButtonStyle == Qt::ToolButtonTextOnly)
	{
		int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
		if (!paintStyle->styleHint(QStyle::SH_UnderlineShortcut, &label, this))
			alignment |= Qt::TextHideMnemonic;
		rect.translate(shiftX, shiftY);
		painter->setFont(label.font);
		doc.drawContents(painter, rect);
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
			painter->setFont(label.font);
			QRect pr = rect,
			tr = rect;
			int alignment = Qt::TextShowMnemonic;
			if (!paintStyle->styleHint(QStyle::SH_UnderlineShortcut, &label, this))
				alignment |= Qt::TextHideMnemonic;

			if (label.toolButtonStyle == Qt::ToolButtonTextUnderIcon)
			{
				pr.setHeight(pmSize.height() + 6);
				tr.adjust(0, pr.height() - 1, 0, -3);
				pr.translate(shiftX, shiftY);
				if (!hasArrow)
				{
					paintStyle->drawItemPixmap(painter, pr, Qt::AlignCenter, pm);
				}
				else
				{
					drawArrow(style(), &label, pr, painter, this);
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
					paintStyle->drawItemPixmap(painter, QStyle::visualRect(label.direction, rect, pr), Qt::AlignCenter, pm);
				}
				else
				{
					drawArrow(style(), &label, pr, painter, this);
				}
				alignment |= Qt::AlignLeft | Qt::AlignVCenter;
			}
			tr.translate(shiftX, shiftY);
			tr = QStyle::visualRect(opt->direction, rect, tr);
			painter->save();
			painter->translate(tr.x(), shiftY);
			rect.translate(shiftX, shiftY);
			doc.drawContents(painter, rect);
			painter->restore();
		}
		else
		{
			rect.translate(shiftX, shiftY);
			if (hasArrow)
			{
				drawArrow(style(), &label, rect, painter, this);
			}
			else
			{
				paintStyle->drawItemPixmap(painter, rect, Qt::AlignCenter, pm);
			}
		}
	}
	// end of label drawing

	if (opt->subControls & QStyle::SC_ToolButtonMenu)
	{
		tool.rect = menuarea;
		tool.state = mflags;
		if (mflags & (QStyle::State_Sunken | QStyle::State_On | QStyle::State_Raised))
			paintStyle->drawPrimitive(QStyle::PE_IndicatorButtonDropDown, &tool, painter, this);
		paintStyle->drawPrimitive(isDown() ? QStyle::PE_IndicatorArrowUp : QStyle::PE_IndicatorArrowDown, &tool, painter, this);
		//painter->drawImage(tool.rect.topLeft(), isDown() ? menuIndicatorUp : menuIndicatorDown);
	}
	else if (opt->features & QStyleOptionToolButton::HasMenu)
	{
		int mbi = 12;//paintStyle->pixelMetric(QStyle::PM_MenuButtonIndicator, opt, this);
		QRect ir = opt->rect;
		QStyleOptionToolButton newBtn = *opt;
		newBtn.rect = QRect(sizeHint().width() - mbi, ir.y() + ir.height() / 2 - 2, mbi - 6, mbi - 6);
		paintStyle->drawPrimitive(isDown() ? QStyle::PE_IndicatorArrowUp : QStyle::PE_IndicatorArrowDown, &newBtn, painter, this);
		//painter->drawImage(newBtn.rect.topLeft(), isDown() ? menuIndicatorUp : menuIndicatorDown);
	}
	painter->end();
	delete painter;
}
