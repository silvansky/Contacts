#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT WidgetManager
{
public:
	static WidgetManager *instance(); // TODO: implement (for signals)
	static void raiseWidget(QWidget *AWidget);
	static void showActivateRaiseWindow(QWidget *AWindow);
	static void setWindowSticky(QWidget *AWindow, bool ASticky);
	static void alertWidget(QWidget *AWidget);
	static bool isWidgetAlertEnabled();
	static void setWidgetAlertEnabled(bool AEnabled);
	static Qt::Alignment windowAlignment(const QWidget *AWindow);
	static bool alignWindow(QWidget *AWindow, Qt::Alignment AAlign);
	static QRect alignRect(const QRect &ARect, const QRect &ABoundary, Qt::Alignment AAlign=Qt::AlignCenter);
	static QRect alignGeometry(const QSize &ASize, const QWidget *AWidget=NULL, Qt::Alignment AAlign=Qt::AlignCenter);
	// TODO: add void animateWindowGeometry(QWidget *window, const QRect &oldGeom, const QRect &newGeom, int duration = -1); - for animating window's geometry
	// TODO: add signal void windowGeometryAnimationFinished(QWidget *window); - emit on window geometry animation finished
	// TODO: add void animateWindowOpacity(QWidget *window, double oldOpacity, double newOpacity, int duration = -1); - for animating window's opacity
	// TODO: add signal void windowOpacityAnimationFinished(QWidget *window); - emit on window opacity animation finished
};

#endif //WIDGETMANAGER_H
