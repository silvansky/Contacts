#ifndef PHONEDIALERDIALOG_H
#define PHONEDIALERDIALOG_H

#include <QDialog>
#include <QSignalMapper>
#include <interfaces/isipphone.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipluginmanager.h>
#include "ui_phonedialerdialog.h"

class PhoneDialerDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	PhoneDialerDialog(IPluginManager *APluginManager, ISipManager *ASipManager, IXmppStream *AXmppStream, QWidget *AParent = NULL);
	~PhoneDialerDialog();
	Jid streamJid() const;
protected:
	void initialize(IPluginManager *APluginManager);
	bool requestBalance();
	bool requestCallCost();
	void updateDialogState();
protected:
	bool isCallEnabled() const;
	QString formattedNumber(const QString &AText) const;
	QString normalizedNumber(const QString &AText) const;
	QString currncyValue(float AValue, const ISipCurrency &ACurrency);
protected slots:
	void onCallButtonClicked();
	void onCostRequestTimerTimeout();
	void onNumberButtonMapped(const QString &AText);
	void onNumberTextChanged(const QString &AText);
protected slots:
	void onSipBalanceRecieved(const Jid &AStreamJid, const ISipBalance &ABalance);
	void onSipCallCostRecieved(const QString &AId, const ISipCallCost &ACost);
private:
	Ui::PhoneDialerDialogClass ui;
private:
	IGateways *FGateways;
	ISipManager *FSipManager;
	IXmppStream *FXmppStream;
	IRosterPlugin *FRosterPlugin;
private:
	QTimer FCostRequestTimer;
	QString FCallCostRequestId;
private:
	ISipBalance FBalance;
	ISipCallCost FCallCost;
	QSignalMapper FNumberMapper;
};

#endif // PHONEDIALERDIALOG_H
