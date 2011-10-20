#ifndef DIALWIDGET_H
#define DIALWIDGET_H

#include <QWidget>
#include <QSignalMapper>
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
protected:
	void updateState();
protected slots:
	void onButtonMapped(const QString &AText);
	void onNumberTextChanged(const QString &AText);
private:
	Ui::DialWidgetClass ui;
private:
	ISipPhone *FSipPhone;
private:
	QSignalMapper FMapper;
};

#endif // DIALWIDGET_H
