#ifndef MAILINFOWIDGET_H
#define MAILINFOWIDGET_H

#include <QWidget>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <utils/stylestorage.h>
#include "ui_mailinfowidget.h"

class MailInfoWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	MailInfoWidget(IChatWindow *AWindow, QWidget *AParent = NULL);
	~MailInfoWidget();
private:
	Ui::MailInfoWidgetClass ui;
};

#endif // MAILINFOWIDGET_H
