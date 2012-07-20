#include "proxystyle.h"

#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <definitions/textflags.h>
#include <definitions/graphicseffects.h>

void ProxyStyle::drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
	if (textRole == QPalette::Text || textRole == QPalette::ButtonText || textRole == QPalette::WindowText)
	{
		// draw the dark shadow by default
		int shadowType = TF_DARKSHADOW;
		if (flags & TF_NOSHADOW)
				shadowType = TF_NOSHADOW;
		if (flags & TF_LIGHTSHADOW)
			shadowType = TF_LIGHTSHADOW;
		if (shadowType != TF_NOSHADOW)
		{
			QGraphicsDropShadowEffect * shadow;
			if (shadowType == TF_DARKSHADOW)
				shadow = qobject_cast<QGraphicsDropShadowEffect*>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(GFX_TEXTSHADOWS));
			else
				shadow = qobject_cast<QGraphicsDropShadowEffect*>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(GFX_NOTICEWIDGET));
			if (shadow)
			{
				QRect shadowRect(rect);
				shadowRect.moveTopLeft(QPoint(rect.left() + shadow->xOffset(), rect.top() + shadow->yOffset()));
				QPalette shadowPal(pal);
				shadowPal.setColor(QPalette::Text, shadow->color());
				QProxyStyle::drawItemText(painter, shadowRect, flags, shadowPal, enabled, text, QPalette::Text);
				shadow->deleteLater();
			}
		}
	}
	QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
}

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
	if (hint == QStyle::SH_EtchDisabledText || hint == QStyle::SH_DitherDisabledText)
		return 0;
	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

void ProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
	case PE_FrameFocusRect:
		break;
	default:
		QProxyStyle::drawPrimitive(element,option,painter,widget);
	}
}
