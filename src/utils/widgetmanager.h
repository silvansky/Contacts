#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT WidgetManager
{
public:
	static WidgetManager *instance();
	static void raiseWidget(QWidget *AWidget);
	static void showActivateRaiseWindow(QWidget *AWindow);
	static void setWindowSticky(QWidget *AWindow, bool ASticky);
};

#endif //WIDGETMANAGER_H
