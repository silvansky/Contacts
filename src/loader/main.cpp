#include <QLibrary>
#include <QApplication>
#include <QProxyStyle>
#include <QDebug>
#include "pluginmanager.h"

// proxy style

class ProxyStyle : public QProxyStyle
{
public:
	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
	{
		qDebug() << "ProxyStyle::drawControl: element == " << element;
		if (element == QStyle::CE_ItemViewItem)
		{
			qDebug() << "ProxyStyle::drawControl: element == QStyle::CE_ItemViewItem";
			if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option))
			{
				QStyleOptionViewItemV4 * vopt2 = new QStyleOptionViewItemV4(*vopt);
				qDebug() << "ProxyStyle::drawControl: vopt";
				qDebug() << vopt->state;
				qDebug() << vopt2->state;
				if (vopt2->state & QStyle::State_HasFocus)
				{
					vopt2->state ^= QStyle::State_HasFocus;
					qDebug() << vopt2->state;
				}
				QProxyStyle::drawControl(element, vopt2, painter, widget);
			}
		}
		else
			QProxyStyle::drawControl(element, option, painter, widget);
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
