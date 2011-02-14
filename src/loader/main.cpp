#include <QLibrary>
#include <QApplication>
#include <QProxyStyle>
#include <QWindowsStyle>
#include <QCleanlooksStyle>
#include <QDebug>
#include <definitions/textflags.h>
#include "pluginmanager.h"

// proxy style

class ProxyStyle : public QProxyStyle
{
public:
	void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
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
				QRect shadowRect(rect);
				shadowRect.moveTopLeft(QPoint(rect.left() + 1, rect.top() - 1));
				QPalette shadowPal(pal);
				shadowPal.setColor(QPalette::Text, QColor(51, 51, 51));
				QProxyStyle::drawItemText(painter, shadowRect, flags, shadowPal, enabled, text, QPalette::Text);
			}
		}
		QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
	}
	int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const
	{
		if (hint == QStyle::SH_EtchDisabledText || hint == QStyle::SH_DitherDisabledText)
			return 0;
		return QProxyStyle::styleHint(hint, option, widget, returnData);
	}
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());
	QApplication::setStyle(new QCleanlooksStyle);
	QApplication::setStyle(new ProxyStyle);
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();
	PluginManager pm(&app);
	pm.restart();
	return app.exec();
}
