#ifndef ACTIONBUTTON_H
#define ACTIONBUTTON_H

#include <QPushButton>
#include "utilsexport.h"
#include "action.h"

class UTILS_EXPORT ActionButton :
	public QPushButton
{
	Q_OBJECT
	Q_PROPERTY(QString action READ actionString WRITE setActionString)
public:
	ActionButton(QWidget *AParent = NULL);
	ActionButton(Action *AAction, QWidget *AParent = NULL);
	Action *action() const;
	void setAction(Action *AAction);
	QString actionString();
	void setActionString(const QString&);
signals:
	void actionChanged();
	void buttonChanged();
private slots:
	void onActionChanged();
	void onActionDestroyed(Action *AAction);
private:
	Action *FAction;
};

#endif // ACTIONBUTTON_H
