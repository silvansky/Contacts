#ifndef CUSTOMLABEL_H
#define CUSTOMLABEL_H

#include <QLabel>

class CustomLabel : public QLabel
{
	Q_OBJECT
public:
	explicit CustomLabel(QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *);
signals:

public slots:

};

#endif // CUSTOMLABEL_H
