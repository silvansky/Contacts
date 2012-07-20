#ifndef SELECTAVATARWIDGET_H
#define SELECTAVATARWIDGET_H

#include <QWidget>
#include "ui_selectavatarwidget.h"

class SelectAvatarWidget : 
	public QWidget 
{
	Q_OBJECT;
public:
	SelectAvatarWidget(QWidget *AParent = NULL);
	~SelectAvatarWidget();
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
signals:
	void avatarSelected(const QImage&);
private:
	Ui::SelectAvatarWidgetClass ui;
};

#endif // SELECTAVATARWIDGET_H
