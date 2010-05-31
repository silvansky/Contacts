#ifndef QHTMLTOOLBUTTON_H
#define QHTMLTOOLBUTTON_H

#include <QToolButton>
#include "utilsexport.h"

class UTILS_EXPORT HtmlToolButton : public QToolButton
{
	Q_OBJECT
public:
	explicit HtmlToolButton(QWidget *parent = 0);
	QString html() const;
	QSize sizeHint() const;
protected:
	void paintEvent(QPaintEvent *);
public slots:
	void setHtml(const QString & html);
private:
};

#endif // QHTMLTOOLBUTTON_H
