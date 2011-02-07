#include <QLibrary>
#include <QApplication>
#include <QProxyStyle>
#include <QDebug>
#include "pluginmanager.h"

// proxy style

class ProxyStyle : public QProxyStyle
{
public:
//	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
//	{
//		if (element == QStyle::CE_ItemViewItem)
//		{
//			qDebug() << "ProxyStyle::drawControl: element == QStyle::CE_ItemViewItem";
//			if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option))
//			{
//				QStyleOptionViewItemV4 * vopt2 = new QStyleOptionViewItemV4(*vopt);
//				qDebug() << "ProxyStyle::drawControl: vopt";
//				qDebug() << vopt->state;
//				qDebug() << vopt2->state;
//				if (vopt2->state & QStyle::State_HasFocus)
//				{
//					vopt2->state ^= QStyle::State_HasFocus;
//					qDebug() << vopt2->state;
//				}
//				QProxyStyle::drawControl(element, vopt2, painter, widget);
//			}
//		}
//		if (element == QStyle::CE_ComboBoxLabel)
//		{
//			qDebug() << "element == QStyle::CE_ComboBoxLabel!";
//			QProxyStyle::drawControl(element, option, painter, widget);
//		}
//		else
//			QProxyStyle::drawControl(element, option, painter, widget);
//	}
	void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
	{
		if (control == CC_ComboBox)
		{
			qDebug() << "Drawing a combo!";
		}
		QProxyStyle::drawComplexControl(control, option, painter, widget);
	}
	void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
	{
		//qDebug() << "drawItemText: " << text << " role: " << textRole;
		if (textRole == QPalette::WindowText || textRole == QPalette::ButtonText)
		{
			QRect shadowRect(rect);
			shadowRect.moveTopLeft(QPoint(rect.left() + 1, rect.top() - 1));
			QPalette shadowPal(pal);
			shadowPal.setColor(QPalette::Text, QColor(51, 51, 51));
			QProxyStyle::drawItemText(painter, shadowRect, flags, shadowPal, enabled, text, QPalette::Text);
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
	QApplication::setStyle(new ProxyStyle);
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();
	PluginManager pm(&app);
	pm.restart();
	return app.exec();
}
