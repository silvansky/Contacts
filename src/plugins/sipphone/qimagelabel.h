#ifndef QIMAGELABEL_H
#define QIMAGELABEL_H

#include <QLabel>
#include <QIcon>
#include <QPicture>

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/toolbargroups.h>
#include <utils/iconstorage.h>

class QImageLabel : public QLabel
{
	Q_OBJECT

public:
	explicit QImageLabel(QWidget *parent = 0);
	enum IconCrossState
	{
		Stable,
		Hover
	};

protected:
	void resizeEvent(QResizeEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
	void leaveEvent(QEvent *);
	//void paintEvent(QPaintEvent *);

private:
	IconStorage * iconStorage;
	QIcon currentIcon;
	QLabel * iconLabel;
	//QPixmap *crossPic;

signals:
	void visibleState(bool);


public slots:
	void updateIcon(IconCrossState iconState);
	virtual void setVisible(bool);
};

#endif // QIMAGELABEL_H
