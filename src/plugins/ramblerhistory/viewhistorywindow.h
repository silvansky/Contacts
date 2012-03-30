#ifndef VIEWHISTORYWINDOW_H
#define VIEWHISTORYWINDOW_H

#include <QMainWindow>
#include <definitions/customborder.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <utils/jid.h>
#include <utils/stylestorage.h>
#include <utils/iconstorage.h>
#include <utils/custombordercontainer.h>
#include <utils/customborderstorage.h>
#include "ui_viewhistorywindow.h"

class ViewHistoryWindow : 
	public QMainWindow
{
	Q_OBJECT;
public:
	ViewHistoryWindow(IRoster *ARoster, IGateways *AGateways, const Jid &AContactJid, QWidget *AParent = NULL);
	~ViewHistoryWindow();
	Jid streamJid() const;
	Jid contactJid() const;
signals:
	void windowDestroyed();
protected:
	void initViewHtml();
	void updateTitle();
protected slots:
	void onWebLoadProgress(int AProgress);
	void onWebLoadFinished(bool AOk);
	void onWebPageLinkClicked(const QUrl &AUrl);
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
private:
	Ui::ViewHistoryWindowClass ui;
private:
	IRoster *FRoster;
	IGateways *FGateways;
private:
	int FProgress;
	Jid FContactJid;
};

#endif // VIEWHISTORYWINDOW_H
