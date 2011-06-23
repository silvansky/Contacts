#include "rosterindexdelegate.h"

#include <QMetaType>
#include <QPainter>
#include <QApplication>
#include <QWindowsVistaStyle>
#include <QLineEdit>
#include <utils/iconstorage.h>
#include <utils/imagemanager.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/textflags.h>
#include <interfaces/ipresence.h>
#include "rostersviewplugin.h"

#define BRANCH_WIDTH  10

//QImage RosterIndexDelegate::groupOpenedIndicator;
//QImage RosterIndexDelegate::groupClosedIndicator;

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{
	FShowBlinkLabels = true;
	//if (groupOpenedIndicator.isNull())
	//	groupOpenedIndicator.load(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_ROSTERVIEW_GROUP_OPENED));
	//if (groupClosedIndicator.isNull())
	//	groupClosedIndicator.load(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_ROSTERVIEW_GROUP_CLOSED));
}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	drawIndex(APainter,AOption,AIndex);
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 option = indexOptions(AIndex,AOption);
	//QStyle *style = option.widget ? option.widget->style() : QApplication::style();
	//	const int hMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin);
	//	const int vMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin);
	const int hMargin = 7;
	const int vMargin = 2;

	QSize leftCenter(0,0);
	QSize middleTop(0,0);
	QSize middleBottom(0,0);
	QSize rightCenter(0,0);

	if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
	{
		leftCenter.rwidth() += BRANCH_WIDTH;
		leftCenter.rheight() += BRANCH_WIDTH;
	}

	QList<LabelItem> labels = itemLabels(AIndex);
	getLabelsSize(option,labels);
	for (QList<LabelItem>::const_iterator it = labels.constBegin(); it!=labels.constEnd(); it++)
	{
		if (it->item.order >= RLAP_LEFT_CENTER && it->item.order < RLAP_LEFT_TOP)
		{
			leftCenter.rwidth() += it->size.width() + spacing;
			leftCenter.rheight() = qMax(leftCenter.height(),it->size.height());
		}
		else if (it->item.order >= RLAP_LEFT_TOP && it->item.order < RLAP_RIGHT_CENTER)
		{
			middleTop.rwidth() += it->size.width() + spacing;
			middleTop.rheight() = qMax(leftCenter.height(),it->size.height());
		}
		else if (it->item.order >= RLAP_RIGHT_CENTER)
		{
			rightCenter.rwidth() += it->size.width() + spacing;
			rightCenter.rheight() = qMax(leftCenter.height(),it->size.height());
		}
	}

	QList<LabelItem> footers = itemFooters(AIndex);
	getLabelsSize(option,footers);
	for (QList<LabelItem>::const_iterator it = footers.constBegin(); it!=footers.constEnd(); it++)
	{
		middleBottom.rwidth() = qMax(middleBottom.width(),it->size.width());
		middleBottom.rheight() += it->size.height();
	}

	QSize hint(0,0);
	hint.rwidth() += qMax(middleTop.width(),middleBottom.width());
	hint.rheight() = qMax(hint.height(),middleTop.height()+middleBottom.height());
	hint.rwidth() += leftCenter.width();
	hint.rheight() = qMax(hint.height(),leftCenter.height());
	hint.rwidth() += rightCenter.width();
	hint.rheight() = qMax(hint.height(),rightCenter.height());
	hint += QSize(hMargin,vMargin);

	switch (AIndex.data(RDR_TYPE).toInt())
	{
	case RIT_GROUP:
	case RIT_GROUP_BLANK:
	case RIT_GROUP_NOT_IN_ROSTER:
	case RIT_GROUP_MY_RESOURCES:
	case RIT_GROUP_AGENTS:
		hint.setHeight(18);
		break;
	case RIT_CONTACT:
	case RIT_METACONTACT:
		if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool() && Options::node(OPV_AVATARS_SHOW).value().toBool())
			hint.setHeight(42);
		else
			hint.setHeight(30);
		break;
	case RIT_SEARCH_EMPTY:
		hint.setHeight(hint.height() * 2);
		break;
	default:
		break;
	}

	return hint;
}

int RosterIndexDelegate::labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	if (!AOption.rect.contains(APoint))
		return RLID_NULL;

	QHash<int,QRect> rectHash = drawIndex(NULL,AOption,AIndex);
	for (QHash<int,QRect>::const_iterator it = rectHash.constBegin(); it != rectHash.constEnd(); it++)
		if (it->contains(APoint))
			return it.key();

	return RLID_DISPLAY;
}

QRect RosterIndexDelegate::labelRect(int ALabelId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	return drawIndex(NULL, AOption, AIndex).value(ALabelId);
}

void RosterIndexDelegate::setShowBlinkLabels(bool AShow)
{
	FShowBlinkLabels = AShow;
}

QWidget *RosterIndexDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	QLineEdit * edit = new QLineEdit(parent);
	edit->setAttribute(Qt::WA_MacShowFocusRect, false);
	return edit;
}

void RosterIndexDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	Q_UNUSED(editor);
	Q_UNUSED(index);
}

QHash<int,QRect> RosterIndexDelegate::drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QHash<int,QRect> rectHash;

	QStyleOptionViewItemV4 option = indexOptions(AIndex,AOption);
	//QStyle *style = option.widget ? option.widget->style() : QApplication::style();

	//#if defined(Q_WS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
	//	if (APainter && qobject_cast<QWindowsVistaStyle *>(QApplication::style()))
	//	{
	//		option.palette.setColor(QPalette::All, QPalette::HighlightedText, option.palette.color(QPalette::Active, QPalette::Text));
	//		option.palette.setColor(QPalette::All, QPalette::Highlight, option.palette.base().color().darker(108));
	//	}
	//#endif
	//	const int hMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin) >> 1;
	//	const int vMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin) >> 1;

	int hMargin = 7;
	int vMargin = 1;

	int labelFlags = TF_NOSHADOW;


	QRect paintRect(option.rect.adjusted(hMargin,vMargin,-hMargin,-vMargin));

	bool isDragged = AIndex.data(RDR_IS_DRAGGED).toBool();
	if (isDragged)
	{
		option.state &= ~(QStyle::State_Selected|QStyle::State_MouseOver);
		paintRect = option.rect;
	}

	if (APainter)
	{
		APainter->save();
		APainter->setClipping(true);
		APainter->setClipRect(option.rect);
		if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
		{
			//labelFlags = TF_DARKSHADOW; // shadow for group names
			if (parent())
			{
				option.backgroundBrush = parent()->property("groupBrush").value<QBrush>();
				// invalid brush, trying image (border image not supported, it is used like BG right now)
				if (option.backgroundBrush.style() == Qt::NoBrush)
				{
					QImage bg = parent()->property("groupBorderImage").value<QImage>();
					APainter->drawImage(option.rect, bg);
				}
				else
				{
					APainter->fillRect(option.rect, option.backgroundBrush);
				}
				QColor c = parent()->property("groupColor").value<QColor>();
				option.palette.setColor(QPalette::Text, c);
				option.palette.setColor(QPalette::HighlightedText, c);
				option.font.setPixelSize(parent()->property("groupFontSize").toInt());
			}
			else
				APainter->fillRect(option.rect, option.backgroundBrush);
		}
		else if (!isDragged)
		{
			drawBackground(APainter, option);
		}

		if (isDragged)
		{
			// draw dragging background
			APainter->save();
			APainter->translate(paintRect.topLeft());
			qreal border = 10.0; // yao magic number
			QImage bg = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_ROSTERVIEW_DRAGGED_ITEM);
			ImageManager::drawNinePartImage(bg, paintRect, border, APainter);
			APainter->restore();
		}
	}

	if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
	{
		QStyleOptionViewItemV4 brachOption(option);
		brachOption.state |= QStyle::State_Children;
		brachOption.rect = QStyle::alignedRect(option.direction, Qt::AlignVCenter | Qt::AlignLeft, QSize(BRANCH_WIDTH, BRANCH_WIDTH), paintRect);
		brachOption.rect.moveTop(brachOption.rect.top() - 1);
		if (APainter && !isDragged)
		{
			APainter->drawImage(brachOption.rect, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(brachOption.state & QStyle::State_Open ? MNI_ROSTERVIEW_GROUP_OPENED : MNI_ROSTERVIEW_GROUP_CLOSED));
		}
		removeWidth(paintRect, BRANCH_WIDTH, AOption.direction == Qt::LeftToRight);
		rectHash.insert(RLID_INDICATORBRANCH, brachOption.rect);
		vMargin += 1;
	}

	QList<LabelItem> labels = itemLabels(AIndex);
	QList<LabelItem> footers = itemFooters(AIndex);
	getLabelsSize(option,labels);
	getLabelsSize(option,footers);
	qSort(labels);
	qSort(footers);

	int leftIndex =0;
	int verticalAdd = AIndex.model()->hasChildren(AIndex) ? 1 : 0;
	for (; leftIndex < labels.count() && labels.at(leftIndex).item.order < RLAP_LEFT_TOP; leftIndex++)
	{
		LabelItem &label = labels[leftIndex];
		Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter;
		label.rect = QStyle::alignedRect(option.direction,align,label.size,paintRect).intersected(paintRect);
		if (verticalAdd)
			label.rect.moveTop(label.rect.top() + verticalAdd);
		removeWidth(paintRect, label.rect.width(), AOption.direction == Qt::LeftToRight);
		if (APainter && !isDragged)
			drawLabelItem(APainter, option, label, labelFlags);
		rectHash.insert(label.id, label.rect);
	}

	int rightIndex = labels.count() - 1;
	for (; rightIndex >= 0 && labels.at(rightIndex).item.order >= RLAP_RIGHT_CENTER; rightIndex--)
	{
		LabelItem &label = labels[rightIndex];
		Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter;
		label.rect = QStyle::alignedRect(option.direction,align,label.size,paintRect).intersected(paintRect);
		if (verticalAdd)
			label.rect.moveTop(label.rect.top() + verticalAdd);
		removeWidth(paintRect,label.rect.width(),AOption.direction!=Qt::LeftToRight);
		if (APainter && !isDragged)
			drawLabelItem(APainter, option, label, labelFlags);
		rectHash.insert(label.id,label.rect);
	}

	int topLabelsWidth = 0;
	QSize middleTop(paintRect.width(),0);
	for (int i = leftIndex; i <= rightIndex; i++)
	{
		const LabelItem &label = labels.at(i);
		middleTop.rheight() = qMax(middleTop.height(),label.size.height());
		if (label.id !=RLID_DISPLAY)
			topLabelsWidth += label.size.width()+spacing;
	}

	QSize middleBottom(paintRect.width(),0);
	for (int i = 0; i < footers.count(); i++)
	{
		const LabelItem &label = footers.at(i);
		middleBottom.rheight() += label.size.height();
	}

	QSize middle(paintRect.width(), middleTop.height() + middleBottom.height());
	paintRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignVCenter,middle,paintRect).intersected(paintRect);
	QRect topRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignTop,middleTop,paintRect).intersected(paintRect);
	QRect bottomRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignBottom,middleBottom,paintRect).intersected(paintRect);

	for (; leftIndex <= rightIndex && labels.at(leftIndex).item.order < RLAP_RIGHT_TOP; leftIndex++)
	{
		LabelItem &label = labels[leftIndex];
		if (label.id == RLID_DISPLAY)
			label.size.rwidth() = qMin(label.size.width(),middleTop.width()-topLabelsWidth);
		Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft;
		label.rect = QStyle::alignedRect(option.direction,align,label.size,topRect).intersected(topRect);
		if (verticalAdd)
			label.rect.moveTop(label.rect.top() + verticalAdd);
		removeWidth(topRect,label.rect.width(),option.direction==Qt::LeftToRight);
		if (APainter && !isDragged)
			drawLabelItem(APainter,option,label, labelFlags);
		rectHash.insert(label.id,label.rect);
	}

	for (; leftIndex <= rightIndex && labels.at(rightIndex).item.order >= RLAP_RIGHT_TOP; rightIndex--)
	{
		LabelItem &label = labels[rightIndex];
		if (label.id == RLID_DISPLAY)
			label.size.rwidth() = qMin(label.size.width(),middleTop.width()-topLabelsWidth);
		Qt::Alignment align = Qt::AlignVCenter | Qt::AlignRight;
		label.rect = QStyle::alignedRect(option.direction,align,label.size,topRect).intersected(topRect);
		if (verticalAdd)
			label.rect.moveTop(label.rect.top() + verticalAdd);
		removeWidth(topRect,label.rect.width(),option.direction!=Qt::LeftToRight);
		if (APainter && !isDragged)
			drawLabelItem(APainter,option,label, labelFlags);
		rectHash.insert(label.id,label.rect);
	}

	for (int i=0; i<footers.count(); i++)
	{
		LabelItem &label = footers[i];
		label.rect = QStyle::alignedRect(option.direction,Qt::AlignTop|Qt::AlignLeft,label.size,bottomRect).intersected(bottomRect);
		bottomRect.setTop(label.rect.bottom());
		if (APainter && !isDragged)
			drawLabelItem(APainter,indexFooterOptions(option), label, labelFlags);
		rectHash.insert(label.id,label.rect);
	}

	if (APainter)
		APainter->restore();

	return rectHash;
}

void RosterIndexDelegate::drawLabelItem(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const LabelItem &ALabel, int flags) const
{
	if (ALabel.rect.isEmpty() || ALabel.item.label.isNull() || ((ALabel.item.flags & IRostersLabel::Blink)>0 && !FShowBlinkLabels))
		return;

	APainter->setClipRect(ALabel.rect);
	QStyle *style = AOption.widget ? AOption.widget->style() : QApplication::style();

	switch (ALabel.item.label.type())
	{
	case QVariant::Pixmap:
	{
		QPixmap pixmap = qvariant_cast<QPixmap>(ALabel.item.label);
		style->drawItemPixmap(APainter,ALabel.rect,Qt::AlignHCenter|Qt::AlignVCenter,pixmap);
		break;
	}
	case QVariant::Image:
	{
		QImage image = qvariant_cast<QImage>(ALabel.item.label);
		APainter->drawImage(ALabel.rect.topLeft(),image);
		break;
	}
	case QVariant::Icon:
	{
		QIcon icon = qvariant_cast<QIcon>(ALabel.item.label);
		QSize sz;// = AOption.decorationSize;
		QList<QSize> availableSizes = icon.availableSizes();
		sz = availableSizes.first();
//		int minh = availableSizes.first().height();
//		int d = abs(sz.height() - minh);
//		foreach (QSize size, availableSizes)
//		{
//			if (abs(size.height() - minh) < d)
//			{
//				d = abs(size.height() - minh);
//				minh = size.height();
//				sz = size;
//			}
//		}

		QPixmap pixmap = style->generatedIconPixmap(getIconMode(AOption.state),icon.pixmap(sz),&AOption);
		style->drawItemPixmap(APainter,ALabel.rect,Qt::AlignHCenter|Qt::AlignVCenter,pixmap);
		break;
	}
	case QVariant::String:
	{
		APainter->setFont(AOption.font);
		int textFlags = AOption.direction | Qt::TextSingleLine;
		QPalette::ColorRole role = AOption.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
		QString text = AOption.fontMetrics.elidedText(prepareText(ALabel.item.label.toString()),Qt::ElideRight,ALabel.rect.width(),textFlags);
		textFlags |= flags;
		QRect textRect(ALabel.rect);
		// dirty hack
		textRect.moveBottom(textRect.bottom() - 1);
		style->drawItemText(APainter, textRect, textFlags, AOption.palette, (AOption.state &  QStyle::State_Enabled) > 0, text, role);
		break;
	}
	default:
		break;
	}
}

void RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AOption) const
{
	QStyle *style = AOption.widget ? AOption.widget->style() : QApplication::style();
	style->drawPrimitive(QStyle::PE_PanelItemViewItem,&AOption,APainter,AOption.widget);
}

void RosterIndexDelegate::drawFocus(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const QRect &ARect) const
{
	if ((AOption.state & QStyle::State_HasFocus) && ARect.isValid())
	{
		QStyleOptionFocusRect focusOption;
		focusOption.QStyleOption::operator=(AOption);
		focusOption.rect = ARect;
		focusOption.state |= QStyle::State_KeyboardFocusChange;
		QPalette::ColorGroup cg = (AOption.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
		QPalette::ColorRole cr = (AOption.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window;
		focusOption.backgroundColor = AOption.palette.color(cg,cr);
		QStyle *style = AOption.widget ? AOption.widget->style() : QApplication::style();
		style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
	}
}

QStyleOptionViewItemV4 RosterIndexDelegate::indexOptions(const QModelIndex &AIndex, const QStyleOptionViewItem &AOption) const
{
	QStyleOptionViewItemV4 option = AOption;

	QVariant data = AIndex.data(Qt::FontRole);
	if (data.isValid())
		option.font = qvariant_cast<QFont>(data).resolve(option.font);

	data = AIndex.data(RDR_FONT_HINT);
	if (data.isValid())
		option.font.setStyleHint((QFont::StyleHint)data.toInt());

	data = AIndex.data(RDR_FONT_SIZE);
	if (data.isValid())
		option.font.setPointSize(data.toInt());

	data = AIndex.data(RDR_FONT_WEIGHT);
	if (data.isValid())
		option.font.setWeight(data.toInt());

	data = AIndex.data(RDR_FONT_STYLE);
	if (data.isValid())
		option.font.setStyle((QFont::Style)data.toInt());

	data = AIndex.data(RDR_FONT_UNDERLINE);
	if (data.isValid())
		option.font.setUnderline(data.toBool());

	data = AIndex.data(RDR_STATES_FORCE_ON);
	if (data.isValid())
		option.state |= (QStyle::State)data.toInt();

	data = AIndex.data(RDR_STATES_FORCE_OFF);
	if (data.isValid())
		option.state &= ~(QStyle::State)data.toInt();

	data = AIndex.data(Qt::ForegroundRole);
	if (qVariantCanConvert<QBrush>(data))
		option.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(data));

	data = AIndex.data(RDR_SHOW);
	int show = data.toInt();
	if (show == IPresence::Offline || show == IPresence::Error)
		if (parent())
			option.palette.setColor(QPalette::Text, parent()->property("footerColor").value<QColor>());

	data = AIndex.data(Qt::BackgroundRole);
	if (qVariantCanConvert<QBrush>(data))
		option.backgroundBrush = qvariant_cast<QBrush>(data);

	option.index = AIndex;
	option.fontMetrics = QFontMetrics(option.font);

	return option;
}

QStyleOptionViewItemV4 RosterIndexDelegate::indexFooterOptions(const QStyleOptionViewItemV4 &AOption) const
{
	QStyleOptionViewItemV4 option = AOption;
	option.font.setBold(false);
	option.font.setPixelSize(option.font.pixelSize() - 1);
	option.fontMetrics = QFontMetrics(option.font);
	if (parent())
		option.palette.setColor(QPalette::Text, parent()->property("footerColor").value<QColor>());
	else
		option.palette.setColor(QPalette::Text, option.palette.color(QPalette::Disabled, QPalette::Text));
	return option;
}

QList<LabelItem> RosterIndexDelegate::itemLabels(const QModelIndex &AIndex) const
{
	QList<LabelItem> labels;
	RostersLabelItems rlItems = AIndex.data(RDR_LABEL_ITEMS).value<RostersLabelItems>();
	for (RostersLabelItems::const_iterator it = rlItems.constBegin(); it != rlItems.constEnd(); it++)
	{
		LabelItem label;
		label.id = it.key();
		label.item = it.value();
		label.item.label = label.item.label.type()==QVariant::Int ? AIndex.data(label.item.label.toInt()) : label.item.label;
		labels.append(label);
	}

	LabelItem decoration;
	decoration.id = RLID_DECORATION;
	decoration.item.order = RLO_DECORATION;
	decoration.item.flags = AIndex.data(RDR_DECORATION_FLAGS).toInt();
	decoration.item.label = AIndex.data(Qt::DecorationRole);
	labels.append(decoration);

	LabelItem display;
	display.id = RLID_DISPLAY;
	display.item.order = RLO_DISPLAY;
	display.item.flags = AIndex.data(RDR_DISPLAY_FLAGS).toInt();
	display.item.label = AIndex.data(Qt::DisplayRole);
	labels.append(display);

	return labels;
}

QList<LabelItem> RosterIndexDelegate::itemFooters(const QModelIndex &AIndex) const
{
	QList<LabelItem> footers;
	QMap<QString,QVariant> footerMap = AIndex.data(RDR_FOOTER_TEXT).toMap();
	QMap<QString,QVariant>::const_iterator fit = footerMap.constBegin();
	while (fit != footerMap.constEnd())
	{
		LabelItem footer;
		footer.id = RLID_FOOTER_TEXT;
		footer.item.order = fit.key().toInt();
		footer.item.flags = 0;
		footer.item.label = fit.value().type()==QVariant::Int ? AIndex.data(fit.value().toInt()) : fit.value();
		footers.append(footer);
		fit++;
	}
	return footers;
}

QSize RosterIndexDelegate::variantSize(const QStyleOptionViewItemV4 &AOption, const QVariant &AValue) const
{
	switch (AValue.type())
	{
	case QVariant::Pixmap:
	{
		QPixmap pixmap = qvariant_cast<QPixmap>(AValue);
		if (!pixmap.isNull())
			return pixmap.size();
		break;
	}
	case QVariant::Image:
	{
		QImage image = qvariant_cast<QImage>(AValue);
		if (!image.isNull())
			return image.size();
		break;
	}
	case QVariant::Icon:
	{
		QIcon icon = qvariant_cast<QIcon>(AValue);
		if (!icon.isNull())
			return AOption.decorationSize;
		break;
	}
	case QVariant::String:
	{
		QString text = prepareText(AValue.toString());
		if (!text.isEmpty())
			return AOption.fontMetrics.size(AOption.direction|Qt::TextSingleLine,text);
		break;
	}
	default:
		break;
	}
	return QSize(0,0);
}

void RosterIndexDelegate::getLabelsSize(const QStyleOptionViewItemV4 &AOption, QList<LabelItem> &ALabels) const
{
	for (QList<LabelItem>::iterator it = ALabels.begin(); it != ALabels.end(); it++)
		it->size = variantSize(it->id==RLID_FOOTER_TEXT ? indexFooterOptions(AOption) : AOption, it->item.label);
}

void RosterIndexDelegate::removeWidth(QRect &ARect,int AWidth, bool AIsLeftToRight) const
{
	if (AIsLeftToRight)
		ARect.setLeft(ARect.left()+AWidth+spacing);
	else
		ARect.setRight(ARect.right()-AWidth-spacing);
}

QString RosterIndexDelegate::prepareText(const QString &AText) const
{
	QString ptext = AText;
	ptext.replace('\n',' ');
	return ptext;
}

QIcon::Mode RosterIndexDelegate::getIconMode(QStyle::State AState) const
{
	//Q_UNUSED(AState)
	if (!(AState & QStyle::State_Enabled))
		return QIcon::Disabled;
	if (AState & QStyle::State_Selected)
		return QIcon::Selected;
	return QIcon::Normal;
}

QIcon::State RosterIndexDelegate::getIconState(QStyle::State AState) const
{
	return AState & QStyle::State_Open ? QIcon::On : QIcon::Off;
}
