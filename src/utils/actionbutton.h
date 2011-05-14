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
	Q_PROPERTY(int textHorizontalAlignment READ textHorizontalAlignment WRITE setTextHorizontalAlignment)
public:
	ActionButton(QWidget *AParent = NULL);
	ActionButton(Action *AAction, QWidget *AParent = NULL);
	Action *action() const;
	void setAction(Action *AAction);
	QString actionString();
	void setActionString(const QString&);
	void addTextFlag(int flag);
	// use 1 for left, 2 for right, 4 for center
	int textHorizontalAlignment() const;
	void setTextHorizontalAlignment(int alignment);
signals:
	void actionChanged();
	void buttonChanged();
private slots:
	void onActionChanged();
	void onActionDestroyed(Action *AAction);
protected:
	void paintEvent(QPaintEvent *);
private:
	Action *FAction;
	int additionalTextFlag;
	int hAlignment;
};

#endif // ACTIONBUTTON_H
