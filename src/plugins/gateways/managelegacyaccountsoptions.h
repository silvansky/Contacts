#ifndef MANAGELEGACYACCOUNTSOPTIONS_H
#define MANAGELEGACYACCOUNTSOPTIONS_H

#include <QWidget>
#include <QVBoxLayout>
#include <interfaces/igateways.h>
#include <interfaces/iroster.h>
#include <interfaces/ioptionsmanager.h>
#include "legacyaccountoptions.h"
#include "ui_managelegacyaccountsoptions.h"

class ManageLegacyAccountsOptions : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ManageLegacyAccountsOptions(IGateways *AGateways, const Jid &AStreamJid, QWidget *AParent = NULL);
	~ManageLegacyAccountsOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void appendServiceOptions(const Jid &AServiceJid);
	void removeServiceOptions(const Jid &AServiceJid);
protected slots:
	void onStreamServicesChanged(const Jid &AStreamJid);
private:
	Ui::ManageLegacyAccountsOptionsClass ui;
private:
	IGateways *FGateways;
private:
	Jid FStreamJid;
	QVBoxLayout *FLayout;
	QMap<Jid, LegacyAccountOptions *> FOptions;
};

#endif // MANAGELEGACYACCOUNTSOPTIONS_H
