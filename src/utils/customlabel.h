#ifndef CUSTOMLABEL_H
#define CUSTOMLABEL_H

#include <QLabel>
#include "utilsexport.h"

class UTILS_EXPORT CustomLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(int shadow READ shadow WRITE setShadow)
	Q_PROPERTY(int elideMode READ elideMode WRITE setElideMode)
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
	Qt::TextElideMode elideMode() const;
	void setElideMode(/*Qt::TextElideMode*/ int mode);
	bool multilineElideEnabled() const;
	void setMultilineElideEnabled(bool on);
protected:
	void paintEvent(QPaintEvent *);
signals:

public slots:
private:
	ShadowType shadowType;
	Qt::TextElideMode textElideMode;
	bool multilineElide;
};

#endif // CUSTOMLABEL_H
