#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include <QLineEdit>
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
	void updateMoodText();

private:
	Ui::StatusWidget *ui;
	bool avatarHovered;
	::SelectAvatarWidget * selectAvatarWidget;
	QString userName;
	QString userMood;
	QLineEdit * moodEditor;
signals:
	void avatarChanged(const QImage &);
public slots:
	void onAvatarChanged(const QImage &);
	void setUserName(const QString& name);
	void setMoodText(const QString& mood);
};

#endif // STATUSWIDGET_H
