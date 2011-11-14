#ifndef SELECTPROFILEWIDGET_H
#define SELECTPROFILEWIDGET_H

#include <QRadioButton>
#include <definitions/optionnodes.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_selectprofilewidget.h"

class SelectProfileWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	SelectProfileWidget(IRoster *ARoster, IGateways *AGateways, IOptionsManager *AOptionsManager, const IGateServiceDescriptor &ADescriptor, QWidget *AParent = NULL);
	~SelectProfileWidget();
	Jid streamJid() const;
	QList<Jid> profiles() const;
	Jid selectedProfile() const;
	void setSelectedProfile(const Jid &AServiceJid);
signals:
	void profilesChanged();
	void selectedProfileChanged();
	void adjustSizeRequested();
protected:
	void updateProfiles();
protected slots:
	void onRosterOpened();
	void onRosterClosed();
	void onProfileButtonToggled(bool AChecked);
	void onProfileLabelLinkActivated(const QString &ALink);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
	void onStreamServicesChanged(const Jid &AStreamJid);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem);
private:
	Ui::SelectProfileWidgetClass ui;
private:
	IRoster *FRoster;
	IGateways *FGateways;
	IOptionsManager *FOptionsManager;
private:
	QMap<QString, Jid> FLoginRequests;
private:
	bool FVisible;
	QList<Jid> FUpdateLogins;
	IGateServiceDescriptor FDescriptor;
	QMap<Jid, QString> FProfileLogins;
	QMap<Jid, QLabel *> FProfileLabels;
	QMap<Jid, QRadioButton *> FProfiles;
};

#endif // SELECTPROFILEWIDGET_H
