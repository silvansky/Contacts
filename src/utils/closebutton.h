#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QAbstractButton>
#include "utilsexport.h"

class UTILS_EXPORT CloseButton :
		public QAbstractButton
{
	Q_OBJECT
public:
	CloseButton(QWidget *AParent);
	virtual QSize sizeHint() const;
protected:
	virtual void enterEvent(QEvent *AEvent);
	virtual void leaveEvent(QEvent *AEvent);
	virtual void paintEvent(QPaintEvent *AEvent);
};

#endif // CLOSEBUTTON_H
