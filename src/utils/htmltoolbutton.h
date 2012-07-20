#ifndef HTMLTOOLBUTTON_H
#define HTMLTOOLBUTTON_H

#include <QToolButton>
#include "utilsexport.h"

class UTILS_EXPORT HtmlToolButton : 
	public QToolButton
{
	Q_OBJECT;
public:
	explicit HtmlToolButton(QWidget *AParent = NULL);
	QString html() const;
	virtual QSize sizeHint() const;
public slots:
	void setHtml(const QString &AHtml);
protected:
	void paintEvent(QPaintEvent *AEvent);
//private:
//	static QImage menuIndicatorUp;
//	static QImage menuIndicatorDown;
};

#endif // HTMLTOOLBUTTON_H
