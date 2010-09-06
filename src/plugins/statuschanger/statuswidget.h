#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <definations/stylesheets.h>
#include <utils/menu.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
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
	void paintEvent(QPaintEvent *);
	bool eventFilter(QObject *, QEvent *);
	void updateMoodText();

private:
	Ui::StatusWidget *ui;
	bool avatarHovered;
	::SelectAvatarWidget * selectAvatarWidget;
	QString userName;
	QString userMood;
	QImage logo;
	QLineEdit * moodEditor;
	Menu * profileMenu;
signals:
	void avatarChanged(const QImage &);
	void moodSet(const QString &);
public slots:
	void setUserName(const QString& name);
	void setMoodText(const QString& mood);
	void startEditMood();
	void finishEditMood();
	void cancelEditMood();
protected slots:
	void profileMenuAboutToHide();
	void profileMenuAboutToShow();
	void onManageProfileTriggered();
	void onAddAvatarTriggered();
private:
	QString fitCaptionToWidth(const QString & name, const QString & status, const int width) const;
};

#endif // STATUSWIDGET_H
