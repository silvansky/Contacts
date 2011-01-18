#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/vcardvaluenames.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/iavatars.h>
#include <interfaces/ivcard.h>
#include <interfaces/imainwindow.h>
#include <utils/menu.h>
#include <utils/message.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include "ui_statuswidget.h"
#include "selectavatarwidget.h"

class StatusWidget :
	public QWidget
{
	Q_OBJECT
public:
	StatusWidget(IStatusChanger *AStatusChanger, IAvatars *AAvatars, IVCardPlugin *AVCardPlugin, IMainWindowPlugin * AMainWindowPlugin, QWidget *AParent = NULL);
	~StatusWidget();
public:
	Jid streamJid() const;
	void setStreamJid(const Jid &AStreamJid);
protected:
	void startEditMood();
	void finishEditMood();
	void cancelEditMood();
	void setUserName(const QString &AName);
	void setMoodText(const QString &AMood);
	QString fitCaptionToWidth(const QString &AName, const QString &AStatus, const int AWidth) const;
protected:
	void resizeEvent(QResizeEvent *);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onAddAvatarTriggered();
	void onManageProfileTriggered();
	void onProfileMenuAboutToHide();
	void onProfileMenuAboutToShow();
	void onVCardReceived(const Jid &AContactJid);
	void onStatusChanged(const Jid &AStreamJid, int AStatusId);
private:
	Ui::StatusWidgetClass ui;
private:
	IAvatars *FAvatars;
	IVCardPlugin *FVCardPlugin;
	IStatusChanger *FStatusChanger;
	IMainWindowPlugin * FMainWindowPlugin;
private:
	bool FAvatarHovered;
	Jid FStreamJid;
	QString FUserName;
	QString FUserMood;
	Menu *FProfileMenu;
	SelectAvatarWidget *FSelectAvatarWidget;
};

#endif // STATUSWIDGET_H
