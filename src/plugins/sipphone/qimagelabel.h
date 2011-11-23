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

	QPoint correctTopLeftPos(const QPoint &APos) const;
protected:
	void resizeEvent(QResizeEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void leaveEvent(QEvent *);
	void paintEvent(QPaintEvent *);

private:
	IconStorage * iconStorage;
	QIcon currentIcon;
	QLabel * iconLabel;
	QPoint pressedPos;

signals:
	void visibleState(bool);
	void moveTo(const QPoint &APoint);


public slots:
	void updateIcon(IconCrossState iconState);
	virtual void setVisible(bool);
	void setPixmap(const QPixmap &);

public:
	static int spacing;
};

#endif // QIMAGELABEL_H
