#ifndef QHTMLTOOLBUTTON_H
#define QHTMLTOOLBUTTON_H

#include <QToolButton>
#include "utilsexport.h"

class UTILS_EXPORT QHtmlToolButton : public QToolButton
{
	Q_OBJECT
public:
	explicit QHtmlToolButton(QWidget *parent = 0);
	QString html() const;
	QSize sizeHint() const;

protected:
	void paintEvent(QPaintEvent *);
signals:
	void htmlChanged(const QString &);

public slots:
	void setHtml(const QString & html);
private:
};

#endif // QHTMLTOOLBUTTON_H
