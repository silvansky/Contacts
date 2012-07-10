#ifndef PHONEDIALERDIALOG_H
#define PHONEDIALERDIALOG_H

#include <QDialog>
#include <QSignalMapper>
#include <QTableWidgetItem>
#include <interfaces/isipphone.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iprivatestorage.h>
#include "ui_phonedialerdialog.h"

struct CallHistoryItem
{
	QString number;
	QDateTime start;
	qint64 duration;
	bool callFailed;
};

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
	void requestBalance();
	void requestCallCost();
	void updateDialogState();
	void showErrorBalloon(const QString &AHtml);
protected:
	Jid findPhoneContact(const QString &ANumber) const;
	QString startTimeString(const QDateTime &AStart) const;
	QString numberContactName(const QString &ANumber) const;
	void prependCallHistory(const CallHistoryItem &AItem);
protected:
	bool isCallEnabled() const;
	QString formattedNumber(const QString &AText) const;
	QString normalizedNumber(const QString &AText) const;
	QString currncyValue(float AValue, const ISipCurrency &ACurrency) const;
protected slots:
	void saveCallHistory();
	void loadCallHistory();
protected slots:
	void onXmppStreamOpened();
	void onXmppStreamClosed();
	void onCallButtonClicked();
	void onCostRequestTimerTimeout();
	void onNumberButtonMapped(const QString &AText);
	void onNumberTextChanged(const QString &AText);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
protected slots:
	void onHistoryCellDoubleClicked(int ARow, int AColumn);
protected slots:
	void onCallWindowDestroyed();
	void onCallStateChanged(int AState);
	void onSipBalanceRecieved(const Jid &AStreamJid, const ISipBalance &ABalance);
	void onSipCallCostRecieved(const QString &AId, const ISipCallCost &ACost);
private:
	Ui::PhoneDialerDialogClass ui;
private:
	IGateways *FGateways;
	ISipManager *FSipManager;
	IXmppStream *FXmppStream;
	IRosterPlugin *FRosterPlugin;
	IMetaContacts *FMetaContacts;
	IPrivateStorage *FPrivateStorage;
private:
	QTimer FCostRequestTimer;
	QString FCallCostRequestId;
private:
	ISipBalance FBalance;
	ISipCallCost FCallCost;
	QSignalMapper FNumberMapper;
private:
	bool FAutoStartCall;
	QList<Jid> FPhoneContacts;
	QMap<QTableWidgetItem *,CallHistoryItem> FCallHistory;
};

#endif // PHONEDIALERDIALOG_H
