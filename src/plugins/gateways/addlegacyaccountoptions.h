#ifndef ADDLEGACYACCOUNTOPTIONS_H
#define ADDLEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <QHBoxLayout>
#include <definations/resources.h>
#include <interfaces/igateways.h>
#include <interfaces/iroster.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/action.h>
#include "ui_addlegacyaccountoptions.h"

class AddLegacyAccountOptions : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	AddLegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, QWidget *AParent=NULL);
	~AddLegacyAccountOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void appendServiceButton(const Jid &AServiceJid);
	void removeServiceButton(const Jid &AServiceJid);
protected slots:
	void onGateActionTriggeted(bool);
	void onServicesChanged(const Jid &AStreamJid);
private:
	Ui::AddLegacyAccountOptionsClass ui;
private:
	IGateways *FGateways;
private:
	Jid FStreamJid;
	QHBoxLayout *FLayout;
	QMap<Jid, QWidget *> FWidgets;
};

#endif // ADDLEGACYACCOUNTOPTIONS_H