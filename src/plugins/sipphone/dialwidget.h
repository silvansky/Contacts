#ifndef DIALWIDGET_H
#define DIALWIDGET_H

#include <QWidget>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/isipphone.h>
#include <utils/stylestorage.h>
#include "ui_dialwidget.h"

class DialWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	DialWidget(ISipPhone *ASipPhone, QWidget *AParent = NULL);
	~DialWidget();
private:
	Ui::DialWidgetClass ui;
private:
	ISipPhone *FSipPhone;
};

#endif // DIALWIDGET_H
