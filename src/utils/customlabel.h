#ifndef CUSTOMLABEL_H
#define CUSTOMLABEL_H

#include <QLabel>
#include "utilsexport.h"

class UTILS_EXPORT CustomLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(int shadow READ shadow WRITE setShadow)
public:
	explicit CustomLabel(QWidget *parent = 0);
	enum ShadowType
	{
		NoShadow = 0,
		DarkShadow = 1,
		LightShadow = 2
	};
	int shadow() const;
	void setShadow(int shadow);

protected:
	void paintEvent(QPaintEvent *);
signals:

public slots:
private:
	ShadowType shadowType;

};

#endif // CUSTOMLABEL_H
