#include "proxystyle.h"

#include <QDebug>
#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <definitions/textflags.h>
#include <definitions/graphicseffects.h>


void ProxyStyle::drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
	qDebug() << "drawItemText: " << text << " role: " << textRole << " flags: " << QString::number(flags, 2);
	if (textRole == QPalette::Text || textRole == QPalette::ButtonText)
	{
		// draw the dark shadow by default
		int shadowType = TF_DARKSHADOW;
		if (flags & TF_NOSHADOW)
				shadowType = TF_NOSHADOW;
		if (flags & TF_LIGHTSHADOW)
			shadowType = TF_LIGHTSHADOW;
		if (shadowType == TF_DARKSHADOW)
		{
			QGraphicsDropShadowEffect * shadow = qobject_cast<QGraphicsDropShadowEffect*>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(GFX_TEXTSHADOWS));
			if (shadow)
			{
				QRect shadowRect(rect);
				shadowRect.moveTopLeft(QPoint(rect.left() + shadow->xOffset(), rect.top() + shadow->yOffset()));
				QPalette shadowPal(pal);
				shadowPal.setColor(QPalette::Text, shadow->color());
				QProxyStyle::drawItemText(painter, shadowRect, flags, shadowPal, enabled, text, QPalette::Text);
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
