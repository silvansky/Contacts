#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include "selectavatarwidget.h"

namespace Ui
{
	class StatusWidget;
}

class StatusWidget : public QWidget
{
	Q_OBJECT
	friend class StatusChanger;
public:
	StatusWidget(QWidget *parent = 0);
	~StatusWidget();

protected:
	void changeEvent(QEvent *e);
	bool eventFilter(QObject *, QEvent *);

private:
	Ui::StatusWidget *ui;
	bool avatarHovered;
	::SelectAvatarWidget * selectAvatarWidget;
signals:
	void avatarChanged(const QImage &);
public slots:
	void onAvatarChanged(const QImage &);
};

#endif // STATUSWIDGET_H
