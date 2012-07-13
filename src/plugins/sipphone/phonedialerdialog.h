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
#include <interfaces/irosterchanger.h>
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
	public QDialog,
	public ISipPhoneDialerDialog
{
	Q_OBJECT;
	Q_INTERFACES(ISipPhoneDialerDialog);
public:
	PhoneDialerDialog(IPluginManager *APluginManager, ISipManager *ASipManager, IXmppStream *AXmppStream, QWidget *AParent = NULL);
	~PhoneDialerDialog();
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual bool isReady() const;
	virtual bool isCallEnabled() const;
	virtual ISipCall *activeCall() const;
	virtual ISipBalance currentBalance() const;
	virtual ISipCallCost currentCallCost() const;
	virtual QString currentNumber() const;
	virtual void setCurrentNumber(const QString &ANumber);
	virtual void startCall();
signals:
	void dialogStateChanged();
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
	QString formattedNumber(const QString &AText) const;
	QString normalizedNumber(const QString &AText) const;
	QString convertTextToNumber(const QString &AText) const;
	QString currencyValue(float AValue, const ISipCurrency &ACurrency) const;
protected slots:
	void saveCallHistory();
	void loadCallHistory();
protected slots:
	void onXmppStreamOpened();
	void onXmppStreamClosed();
	void onCallButtonClicked();
	void onShowWindowTimerTimeout();
	void onCostRequestTimerTimeout();
	void onNumberButtonMapped(const QString &AText);
	void onNumberTextChanged(const QString &AText);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
protected slots:
	void onAddContactByAction();
	void onStartAutoCallByAction();
	void onHistoryCellDoubleClicked(int ARow, int AColumn);
	void onHistoryCustomContextMenuRequested(const QPoint &APos);
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
	IRosterChanger *FRosterChanger;
	IPrivateStorage *FPrivateStorage;
private:
	QTimer FCostRequestTimer;
	QString FCallCostRequestId;
private:
	ISipCall *FActiveCall;
	ISipBalance FBalance;
	ISipCallCost FCallCost;
	QSignalMapper FNumberMapper;
private:
	bool FAutoStartCall;
	QTimer FShowWindowTimer;
	QList<Jid> FPhoneContacts;
	QMap<QTableWidgetItem *,CallHistoryItem> FCallHistory;
};

#endif // PHONEDIALERDIALOG_H
