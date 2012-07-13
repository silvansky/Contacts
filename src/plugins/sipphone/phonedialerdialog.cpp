#include "phonedialerdialog.h"

#include <QString>
#include <QCompleter>
#include <QClipboard>
#include <QDataStream>
#include <QApplication>
#include <QRegExpValidator>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylevalues.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <definitions/gateserviceidentifiers.h>
#include <utils/log.h>
#include <utils/datetime.h>
#include <utils/balloontip.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>

#define MAX_HISTORY_ROWS          8

#define PST_CALL_HISTORY          "calls"
#define PSN_CALL_HISTORY          "rambler:sipphone:calls-history"

#define ADR_NUMBER                Action::DR_Parametr1

static const QString SubsStringKey   = "0123456789abcdefghijklmnopqrstuvwxyz*#";
static const QString SubsStringValue = "012345678922233344455566677778889999*#";

enum HistoryTableColumns {
	HTC_NAME,
	HTC_START,
	HTC_COUNT
};

PhoneDialerDialog::PhoneDialerDialog(IPluginManager *APluginManager, ISipManager *ASipManager, IXmppStream *AXmppStream, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_PHONEDIALERDIALOG);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setMovable(true);
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
	}
	else
	{
		ui.lblCaption->setVisible(false);
		setAttribute(Qt::WA_DeleteOnClose,true);
	}

	FSipManager = ASipManager;
	FXmppStream = AXmppStream;

	FActiveCall = NULL;
	FAutoStartCall = false;

	FGateways = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = NULL;
	FRosterChanger = NULL;
	FPrivateStorage = NULL;
	FMessageProcessor = NULL;
	initialize(APluginManager);

	window()->setWindowTitle(tr("Calls"));

	static const QString numberTmpl = "<span style=\"color:white;font-size:12px;\">&nbsp;%1</span><span style=\"color:#acacac;font-family:'Courier New';font-size:10px;\">%2</span>";
	
	ui.pbtNumber_1->setHtml(numberTmpl.arg("1","&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
	FNumberMapper.setMapping(ui.pbtNumber_1,"1");
	connect(ui.pbtNumber_1,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_2->setHtml(numberTmpl.arg("2","&nbsp;&nbsp;&nbsp;ABC"));
	FNumberMapper.setMapping(ui.pbtNumber_2,"2");
	connect(ui.pbtNumber_2,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_3->setHtml(numberTmpl.arg("3","&nbsp;&nbsp;&nbsp;DEF"));
	FNumberMapper.setMapping(ui.pbtNumber_3,"3");
	connect(ui.pbtNumber_3,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_4->setHtml(numberTmpl.arg("4","&nbsp;&nbsp;&nbsp;GHI"));
	FNumberMapper.setMapping(ui.pbtNumber_4,"4");
	connect(ui.pbtNumber_4,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_5->setHtml(numberTmpl.arg("5","&nbsp;&nbsp;&nbsp;JKL"));
	FNumberMapper.setMapping(ui.pbtNumber_5,"5");
	connect(ui.pbtNumber_5,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	
	ui.pbtNumber_6->setHtml(numberTmpl.arg("6","&nbsp;&nbsp;&nbsp;MNO"));
	FNumberMapper.setMapping(ui.pbtNumber_6,"6");
	connect(ui.pbtNumber_6,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_7->setHtml(numberTmpl.arg("7","&nbsp;&nbsp;PQRS"));
	FNumberMapper.setMapping(ui.pbtNumber_7,"7");
	connect(ui.pbtNumber_7,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_8->setHtml(numberTmpl.arg("8","&nbsp;&nbsp;&nbsp;TUV"));
	FNumberMapper.setMapping(ui.pbtNumber_8,"8");
	connect(ui.pbtNumber_8,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_9->setHtml(numberTmpl.arg("9","&nbsp;&nbsp;WXYZ"));
	FNumberMapper.setMapping(ui.pbtNumber_9,"9");
	connect(ui.pbtNumber_9,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	
	ui.pbtNumber_10->setHtml(numberTmpl.arg("*","&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
	FNumberMapper.setMapping(ui.pbtNumber_10,"*");
	connect(ui.pbtNumber_10,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));

	ui.pbtNumber_11->setHtml(numberTmpl.arg("0","&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
	FNumberMapper.setMapping(ui.pbtNumber_11,"0");
	connect(ui.pbtNumber_11,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	
	ui.pbtNumber_12->setHtml(numberTmpl.arg("#","&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
	FNumberMapper.setMapping(ui.pbtNumber_12,"#");
	connect(ui.pbtNumber_12,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	connect(&FNumberMapper,SIGNAL(mapped(const QString &)),SLOT(onNumberButtonMapped(const QString &)));

	ui.lblCity->setElideMode(Qt::ElideRight);
	ui.lblCost->setElideMode(Qt::ElideRight);
	ui.lblBalance->setElideMode(Qt::ElideRight);
	ui.lblFullHistory->setElideMode(Qt::ElideRight);
	ui.lblFullHistory->setText(QString("<a href='http://contacts.rambler.ru'><span style='text-decoration: underline; color:%1;'>%2</span></a>")
		.arg(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_DARK_LINK_COLOR).name(), tr("Complete calls history")));

	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid()) : NULL;
	if (FGateways && roster)
	{
		QList<Jid> phoneServices = 
			FGateways->gateDescriptorServices(streamJid(),FGateways->gateDescriptorById(GSID_SMS),true) + 
			FGateways->gateDescriptorServices(streamJid(),FGateways->gateDescriptorById(GSID_PHONE),true);

		foreach(IRosterItem ritem, roster->rosterItems())
		{
			if (phoneServices.contains(ritem.itemJid.domain()))
				FPhoneContacts.append(ritem.itemJid);
		}
	}

	if (FGateways)
	{
		QString number = qApp->clipboard()->text();
		foreach(IGateServiceDescriptor descriptor, FGateways->gateAvailDescriptorsByContact(number))
		{
			if (descriptor.id == GSID_PHONE)
			{
				ui.lneNumber->setText(normalizedNumber(number));
				break;
			}
		}
	}

	FShowWindowTimer.setSingleShot(true);
	FShowWindowTimer.setInterval(2000);
	connect(&FShowWindowTimer,SIGNAL(timeout()),SLOT(onShowWindowTimerTimeout()));

	FCostRequestTimer.setSingleShot(true);
	FCostRequestTimer.setInterval(1000);
	connect(&FCostRequestTimer,SIGNAL(timeout()),SLOT(onCostRequestTimerTimeout()));

	connect(ui.pbtCall,SIGNAL(clicked()),SLOT(onCallButtonClicked()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtCall,MNI_SIPPHONE_DIALER_CALL);

	connect(ui.tbwHistory,SIGNAL(cellDoubleClicked(int,int)),SLOT(onHistoryCellDoubleClicked(int,int)));
	connect(ui.tbwHistory,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onHistoryCustomContextMenuRequested(const QPoint &)));

	connect(FXmppStream->instance(),SIGNAL(opened()),SLOT(onXmppStreamOpened()));
	connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));

	connect(FSipManager->instance(),SIGNAL(sipBalanceRecieved(const Jid &, const ISipBalance &)),SLOT(onSipBalanceRecieved(const Jid &, const ISipBalance &)));
	connect(FSipManager->instance(),SIGNAL(sipCallCostRecieved(const QString &, const ISipCallCost &)),SLOT(onSipCallCostRecieved(const QString &, const ISipCallCost &)));

	connect(ui.lneNumber,SIGNAL(textChanged(const QString &)),SLOT(onNumberTextChanged(const QString &)));

	if (FXmppStream->isOpen())
		onXmppStreamOpened();
	else
		onXmppStreamClosed();
}

PhoneDialerDialog::~PhoneDialerDialog()
{
	BalloonTip::hideBalloon();
}

Jid PhoneDialerDialog::streamJid() const
{
	return FXmppStream->streamJid();
}

bool PhoneDialerDialog::isCallEnabled() const
{
	return isReady() && FBalance.balance>SIP_BALANCE_EPSILON && FCallCost.cost>SIP_BALANCE_EPSILON && FCallCost.cost<=FBalance.balance;
}

ISipCall *PhoneDialerDialog::activeCall() const
{
	return FActiveCall;
}

ISipBalance PhoneDialerDialog::currentBalance() const
{
	return FBalance;
}

bool PhoneDialerDialog::isReady() const
{
	return FActiveCall==NULL;
}

ISipCallCost PhoneDialerDialog::currentCallCost() const
{
	return FCallCost;
}

QString PhoneDialerDialog::currentNumber() const
{
	return normalizedNumber(ui.lneNumber->text());
}

void PhoneDialerDialog::setCurrentNumber(const QString &ANumber)
{
	if (isReady())
		ui.lneNumber->setText(normalizedNumber(ANumber));
}

void PhoneDialerDialog::startCall()
{
	if (isReady())
	{
		LogDetail(QString("[PhoneDialerDialog] Call auto start requested with number='%1'").arg(currentNumber()));
		if (!isCallEnabled())
		{
			FAutoStartCall = true;
			requestCallCost();
			FShowWindowTimer.start();
		}
		else
		{
			onCallButtonClicked();
		}
	}
}

void PhoneDialerDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IGateways").value(0);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
		if (FGateways)
		{
			connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),SLOT(onGateUserJidReceived(const QString &, const Jid &)));
			connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),SLOT(onGateErrorReceived(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0);
	if (plugin)
	{
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}
}

void PhoneDialerDialog::requestBalance()
{
	FBalance = ISipBalance();
	if (!FSipManager->requestBalance(streamJid()))
		FBalance.error = ErrorHandler(ErrorHandler::SERVICE_UNAVAILABLE);
	updateDialogState();
}

void PhoneDialerDialog::requestCallCost()
{
	FCostRequestTimer.stop();
	FCallCost = ISipCallCost();
	QString number = currentNumber();
	if (!number.isEmpty())
	{
		FCallCostRequestId = FSipManager->requestCallCost(streamJid(),FBalance.currency.code,number,QDateTime::currentDateTime(),60000);
		if (FCallCostRequestId.isEmpty())
			FCallCost.error = ErrorHandler(ErrorHandler::SERVICE_UNAVAILABLE);
	}
	updateDialogState();
}

void PhoneDialerDialog::updateDialogState()
{
	QString balanceTempl = QString("<a href='http://contacts.rambler.ru'><span style='text-decoration: underline; color:%1;'>%2</span></a>")
		.arg(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_LINK_COLOR).name());

	if (FBalance.balance > -SIP_BALANCE_EPSILON)
		ui.lblBalance->setText(balanceTempl.arg(currencyValue(FBalance.balance,FBalance.currency)+"."));
	else if (!FXmppStream->isOpen())
		ui.lblBalance->setText(balanceTempl.arg(tr("Disconnected")));
	else if (!FBalance.error.condition().isEmpty())
		ui.lblBalance->setText(balanceTempl.arg(tr("Service is temporarily unavailable")));
	else
		ui.lblBalance->setText(balanceTempl.arg(tr("Updating...")));

	QString costErrCond = FCallCost.error.condition();
	if (FCallCost.cost > -SIP_BALANCE_EPSILON)
	{
		ui.lblCity->setText(!FCallCost.city.isEmpty() ? FCallCost.city +", "+ FCallCost.country : FCallCost.country);
		ui.lblCost->setText(currencyValue(FCallCost.cost,FCallCost.currency)+"/"+tr("min")+".");
	}
	else if (!FCallCostRequestId.isEmpty())
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(tr("Requesting call cost..."));
	}
	else if (costErrCond=="invalid-phone-number" || costErrCond=="incomplete-phone-number")
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(tr("Unsupported number"));
	}
	else if (!costErrCond.isEmpty())
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(tr("Service is temporarily unavailable"));
	}
	else
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(QString::null);
	}

	ui.pbtCall->setEnabled(isCallEnabled());

	emit dialogStateChanged();
}

void PhoneDialerDialog::showErrorBalloon(const QString &AText)
{
	if (window()->isActiveWindow())
	{
		QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_DIALER_ERROR);
		QPoint point = ui.lneNumber->mapToGlobal(ui.lneNumber->rect().topRight());
		point.setY(point.y() + ui.lneNumber->height() / 2);
		BalloonTip::showBalloon(icon,QString::null,AText,point,0,true,BalloonTip::ArrowLeft,window());
		LogDetail(QString("[PhoneDialerDialog] Error balloon shown with message='%1'").arg(AText));
	}
}

void PhoneDialerDialog::showSendSmsDialog(const Jid &APhoneJid)
{
	if (FMessageProcessor)
	{
		LogDetail(QString("[PhoneDialerDialog] Opening SMS dialog for phoneJid='%1'").arg(APhoneJid.full()));
		FMessageProcessor->createMessageWindow(streamJid(),APhoneJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
}

void PhoneDialerDialog::showSendSmsDialog(const QString &ANumber)
{
	if (FGateways)
	{
		Jid phoneJid = findPhoneContact(ANumber);
		if (phoneJid.isEmpty())
		{
			Jid gateJid = FGateways->gateDescriptorServices(streamJid(),FGateways->gateDescriptorById(GSID_SMS),true).value(0);
			if (FGateways->isServiceEnabled(streamJid(),gateJid))
			{
				QString requestId = FGateways->sendUserJidRequest(streamJid(),gateJid,ANumber);
				if (!requestId.isEmpty())
				{
					LogDetail(QString("[PhoneDialerDialog] PhoneJid request for number='%1' sent, id='%2'").arg(ANumber,requestId));
					FSmsJidRequests.append(requestId);
				}
				else
				{
					LogDetail(QString("[PhoneDialerDialog] Failed to send phoneJid request for number='%1'").arg(ANumber));
				}
			}
			else
			{
				LogDetail(QString("[PhoneDialerDialog] Failed to request phoneJid for number='%1': SMS service is not enabled").arg(ANumber));
			}
		}
		else
		{
			showSendSmsDialog(phoneJid);
		}
	}
}

Jid PhoneDialerDialog::findPhoneContact(const QString &ANumber) const
{
	QString number = normalizedNumber(ANumber);
	for (QList<Jid>::const_iterator it=FPhoneContacts.constBegin(); it!=FPhoneContacts.constEnd(); ++it)
	{
		if (normalizedNumber(it->uNode()) == number)
			return *it;
	}
	return Jid::null;
}

QString PhoneDialerDialog::startTimeString(const QDateTime &AStart) const
{
	int daysLeft = AStart.date().daysTo(QDateTime::currentDateTime().date());
	if (daysLeft == 0)
	{
		return AStart.time().toString("h:mm");
	}
	else if (daysLeft == 1)
	{
		return tr("Yesterday");
	}
	else if (daysLeft < 7)
	{
		return QDate::longDayName(AStart.date().dayOfWeek());
	}
	else
	{
		return AStart.date().toString("d MMM");
	}
}

QString PhoneDialerDialog::numberContactName(const QString &ANumber) const
{
	Jid phoneJid = findPhoneContact(ANumber);
	if (phoneJid.isValid())
	{
		IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(streamJid()) : NULL;
		QString metaId = mroster!=NULL ? mroster->itemMetaContact(phoneJid) : QString::null;
		if (!metaId.isEmpty())
			return FMetaContacts->metaContactName(mroster->metaContact(metaId));

		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid()) : NULL;
		IRosterItem ritem = roster!=NULL ? roster->rosterItem(phoneJid) : IRosterItem();
		if (!ritem.name.isEmpty())
			return ritem.name;
	}
	return formattedNumber(ANumber);
}

void PhoneDialerDialog::prependCallHistory(const CallHistoryItem &AItem)
{
	if (!AItem.number.isEmpty() && AItem.start.isValid())
	{
		LogDetail(QString("[PhoneDialerDialog] Prepending call history item, number='%1', start='%2', duration='%3', failed='%4'").arg(AItem.number,AItem.start.toString()).arg(AItem.duration).arg(AItem.callFailed));

		for (QMap<QTableWidgetItem *, CallHistoryItem>::iterator it=FCallHistory.begin(); it!=FCallHistory.end(); )
		{
			if (it->number == AItem.number)
			{
				ui.tbwHistory->removeRow(it.key()->row());
				it = FCallHistory.erase(it);
			}
			else
			{
				++it;
			}
		}

		QTableWidgetItem *itemName = new QTableWidgetItem;
		itemName->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		itemName->setText(numberContactName(AItem.number));
		itemName->setToolTip(formattedNumber(AItem.number));
		if (AItem.callFailed)
		{
			itemName->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_MISSED));
			itemName->setTextColor(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_SDD_HISTORY_FAILED_COLOR));
		}
		else
		{
			itemName->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_OUT));
		}

		QTableWidgetItem *itemStart = new QTableWidgetItem;
		itemStart->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		itemStart->setText(startTimeString(AItem.start));
		itemStart->setTextColor(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_SDD_HISTORY_START_COLOR).name());

		if (ui.tbwHistory->rowCount() == MAX_HISTORY_ROWS)
		{
			QTableWidgetItem *oldItem = ui.tbwHistory->item(ui.tbwHistory->rowCount()-1,HTC_NAME);
			FCallHistory.remove(oldItem);
			ui.tbwHistory->removeRow(ui.tbwHistory->rowCount()-1);
		}
		ui.tbwHistory->setRowCount(ui.tbwHistory->rowCount()+1);

		for (int row=ui.tbwHistory->rowCount()-1; row>0; row--)
			for (int col=0; col<HTC_COUNT; col++)
				ui.tbwHistory->setItem(row,col,ui.tbwHistory->takeItem(row-1,col));

		ui.tbwHistory->setItem(0,HTC_NAME,itemName);
		ui.tbwHistory->setItem(0,HTC_START,itemStart);
		FCallHistory.insert(itemName,AItem);
	}
}

QString PhoneDialerDialog::formattedNumber(const QString &AText) const
{
	QString number = FGateways!=NULL ? FGateways->formattedContactLogin(FGateways->gateDescriptorById(GSID_PHONE),AText) : AText;
	return number;
}

QString PhoneDialerDialog::normalizedNumber(const QString &AText) const
{
	QString number = FGateways!=NULL ? FGateways->normalizedContactLogin(FGateways->gateDescriptorById(GSID_PHONE),AText) : AText;
	return number;
}

QString PhoneDialerDialog::convertTextToNumber(const QString &AText) const
{
	static const QString SubsStringKey   = "0123456789abcdefghijklmnopqrstuvwxyz *#+-()";
	static const QString SubsStringValue = "012345678922233344455566677778889999 *#+-()";

	QString number = AText;
	for (int i=0; i<number.length(); )
	{
		int index = SubsStringKey.indexOf(number.at(i).toLower());
		if (index >= 0)
		{
			number[i] = SubsStringValue[index];
			i++;
		}
		else
		{
			number.remove(i,1);
		}
	}
	return number;
}

QString PhoneDialerDialog::currencyValue(float AValue, const ISipCurrency &ACurrency) const
{
	QString currency = ACurrency.code.toLower();
	return QString("%1 %2").arg(AValue,0,'f',qMax(-ACurrency.exp,0)).arg(currency);
}

void PhoneDialerDialog::saveCallHistory()
{
	if (FPrivateStorage && FPrivateStorage->isOpen(streamJid()))
	{
		QDomDocument doc;
		QDomElement rootElem = doc.appendChild(doc.createElement("history")).appendChild(doc.createElementNS(PSN_CALL_HISTORY,PST_CALL_HISTORY)).toElement();
		for (int row=0; row<ui.tbwHistory->rowCount(); row++)
		{
			QDomElement elem = rootElem.appendChild(doc.createElement("number")).toElement();
			CallHistoryItem historyItem = FCallHistory.value(ui.tbwHistory->item(row,HTC_NAME));
			elem.appendChild(doc.createTextNode(historyItem.number));
			elem.setAttribute("start",DateTime(historyItem.start).toX85DateTime());
			elem.setAttribute("duration",historyItem.duration);
			elem.setAttribute("failed",historyItem.callFailed);
		}
		LogDetail(QString("[PhoneDialerDialog] Saving call history of account='%1'").arg(streamJid().bare()));
		if (FPrivateStorage->saveData(streamJid(),rootElem).isEmpty())
			LogError(QString("[PhoneDialerDialog] Failed to save call history of account='%1'").arg(streamJid().bare()));
	}
}

void PhoneDialerDialog::loadCallHistory()
{
	if (FPrivateStorage && FPrivateStorage->isOpen(streamJid()))
	{
		LogDetail(QString("[PhoneDialerDialog] Loading call history of account='%1'").arg(streamJid().bare()));
		if (FPrivateStorage->loadData(streamJid(),PST_CALL_HISTORY,PSN_CALL_HISTORY).isEmpty())
			LogError(QString("[PhoneDialerDialog] Failed to send load request for call history of account='%1' ").arg(streamJid().bare()));
	}
}

void PhoneDialerDialog::onXmppStreamOpened()
{
	loadCallHistory();
	requestBalance();
}

void PhoneDialerDialog::onXmppStreamClosed()
{
	FBalance = ISipBalance();
	FCallCost = ISipCallCost();
	updateDialogState();
}

void PhoneDialerDialog::onCallButtonClicked()
{
	if (isCallEnabled())
	{

		QString number = FCallCost.number;
		if (number.startsWith('+'))
			number.remove(0,1);

		Jid phoneJid(number,SIPPHONE_SERVICE_JID,QString::null);
		LogDetail(QString("[PhoneDialerDialog] Starting call to phoneJid='%1'").arg(phoneJid.full()));
		FActiveCall = FSipManager->newCall(streamJid(),phoneJid);
		if (FActiveCall)
		{
			FShowWindowTimer.stop();
			QWidget *callWindow = FSipManager->showCallWindow(FActiveCall);
			connect(callWindow,SIGNAL(destroyed()),SLOT(onCallWindowDestroyed()));
			connect(FActiveCall->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
			FActiveCall->startCall();
			window()->hide();
		}
		else
		{
			LogError(QString("[PhoneDialerDialog] Failed to create SipCall for phoneJid='%1'").arg(phoneJid.full()));
		}
		updateDialogState();
	}
}

void PhoneDialerDialog::onShowWindowTimerTimeout()
{
	if (isReady())
		WidgetManager::showActivateRaiseWindow(window());
}

void PhoneDialerDialog::onCostRequestTimerTimeout()
{
	requestCallCost();
}

void PhoneDialerDialog::onNumberButtonMapped(const QString &AText)
{
	ui.lneNumber->insert(AText);
	ui.lneNumber->setFocus();
}

void PhoneDialerDialog::onNumberTextChanged(const QString &AText)
{
	QString number = convertTextToNumber(AText);
	if (number != AText)
		ui.lneNumber->setText(number);
	number = normalizedNumber(number);

	if (FCallCost.number != number)
	{
		FAutoStartCall = false;
		FCallCost = ISipCallCost();
		updateDialogState();
		FCostRequestTimer.start();
		BalloonTip::hideBalloon();
	}
}

void PhoneDialerDialog::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AStreamJid==streamJid() && AElement.tagName()==PST_CALL_HISTORY && AElement.namespaceURI()==PSN_CALL_HISTORY)
	{
		FCallHistory.clear();
		ui.tbwHistory->clear();
		ui.tbwHistory->setRowCount(0);
		ui.tbwHistory->setColumnCount(HTC_COUNT);
		ui.tbwHistory->horizontalHeader()->setResizeMode(HTC_NAME,QHeaderView::Stretch);
		ui.tbwHistory->horizontalHeader()->setResizeMode(HTC_START,QHeaderView::ResizeToContents);

		QMap<QDateTime, CallHistoryItem> historyItems;
		QDomElement elem = AElement.firstChildElement("number");
		while(!elem.isNull())
		{
			CallHistoryItem historyItem;
			historyItem.number = elem.text();
			historyItem.start = DateTime(elem.attribute("start")).toLocal();
			historyItem.duration = elem.attribute("duration").toInt();
			historyItem.callFailed = QVariant(elem.attribute("failed")).toBool();
			historyItems.insert(historyItem.start,historyItem);
			elem = elem.nextSiblingElement("number");
		}

		LogDetail(QString("[PhoneDialerDialog] Call history of account='%1' loaded").arg(AStreamJid.bare()));
		for (QMap<QDateTime, CallHistoryItem>::const_iterator it=historyItems.constBegin(); ui.tbwHistory->rowCount()<MAX_HISTORY_ROWS && it!=historyItems.constEnd(); ++it)
			prependCallHistory(it.value());
	}
}

void PhoneDialerDialog::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (AStreamJid==streamJid() && ATagName==PST_CALL_HISTORY && ANamespace==PSN_CALL_HISTORY)
	{
		LogDetail(QString("[PhoneDialerDialog] Call history of account='%1' changed").arg(AStreamJid.bare()));
		loadCallHistory();
	}
}

void PhoneDialerDialog::onAddContactByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (FRosterChanger && action)
	{
		QWidget *widget = FRosterChanger->showAddContactDialog(streamJid());
		IAddContactDialog *dialog = qobject_cast<IAddContactDialog *>(CustomBorderStorage::isBordered(widget) ? CustomBorderStorage::widgetBorder(widget)->widget() : widget);
		if (dialog)
		{
			dialog->setContactText(action->data(ADR_NUMBER).toString());
			dialog->executeRequiredContactChecks();
		}
	}
}

void PhoneDialerDialog::onStartAutoCallByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		setCurrentNumber(action->data(ADR_NUMBER).toString());
		startCall();
	}
}

void PhoneDialerDialog::onShowSendSmsDialogByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		showSendSmsDialog(action->data(ADR_NUMBER).toString());
	}
}

void PhoneDialerDialog::onHistoryCellDoubleClicked(int ARow, int AColumn)
{
	Q_UNUSED(AColumn);
	CallHistoryItem historyItem = FCallHistory.value(ui.tbwHistory->item(ARow,HTC_NAME));
	setCurrentNumber(historyItem.number);
	startCall();
}

void PhoneDialerDialog::onHistoryCustomContextMenuRequested(const QPoint &APos)
{
	QTableWidgetItem *tableItem = ui.tbwHistory->itemAt(APos);
	if (tableItem)
	{
		CallHistoryItem historyItem = FCallHistory.value(ui.tbwHistory->item(tableItem->row(),HTC_NAME));

		Menu *menu = new Menu(ui.tbwHistory);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);

		if (FRosterChanger && findPhoneContact(historyItem.number).isEmpty())
		{
			Action *addAction = new Action(menu);
			addAction->setText(tr("Add Contact..."));
			addAction->setData(ADR_NUMBER,historyItem.number);
			connect(addAction,SIGNAL(triggered()),SLOT(onAddContactByAction()));
			menu->addAction(addAction);
		}

		Action *callAction = new Action(menu);
		callAction->setText(tr("Call"));
		callAction->setData(ADR_NUMBER,historyItem.number);
		connect(callAction,SIGNAL(triggered()),SLOT(onStartAutoCallByAction()));
		menu->addAction(callAction);

		if (FGateways && FGateways->gateDescriptorStatus(streamJid(),FGateways->gateDescriptorById(GSID_SMS))==IGateways::GDS_ENABLED)
		{
			Action *smsAction = new Action(menu);
			smsAction->setText(tr("Send SMS"));
			smsAction->setData(ADR_NUMBER,historyItem.number);
			connect(smsAction,SIGNAL(triggered()),SLOT(onShowSendSmsDialogByAction()));
			menu->addAction(smsAction);
		}

		menu->popup(mapToGlobal(APos));
	}
}

void PhoneDialerDialog::onGateUserJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FSmsJidRequests.contains(AId))
	{
		LogDetail(QString("[PhoneDialerDialog] PhoneJid='%1' for SMS dialog received, id='%2'").arg(AUserJid.full(),AId));
		if (AUserJid.isValid())
			showSendSmsDialog(AUserJid);
		FSmsJidRequests.removeAll(AId);
	}
}

void PhoneDialerDialog::onGateErrorReceived(const QString &AId, const QString &AError)
{
	if (FSmsJidRequests.contains(AId))
	{
		LogError(QString("[PhoneDialerDialog] Failed to receive phoneJid for SMS dialog, id='%1', error='%2'").arg(AId,AError));
		FSmsJidRequests.removeAll(AId);
	}
}

void PhoneDialerDialog::onCallWindowDestroyed()
{
	requestBalance();
	WidgetManager::showActivateRaiseWindow(window());
}

void PhoneDialerDialog::onCallStateChanged(int AState)
{
	if (AState==ISipCall::CS_FINISHED || AState==ISipCall::CS_ERROR)
	{
		LogDetail(QString("[PhoneDialerDialog] Active call state changed to %1").arg(AState));
		ISipCall *call = qobject_cast<ISipCall *>(sender());
		if (FActiveCall!=NULL && call==FActiveCall)
		{
			if (AState == ISipCall::CS_FINISHED)
				setCurrentNumber(QString::null);

			CallHistoryItem histItem;
			histItem.callFailed = AState==ISipCall::CS_ERROR;
			histItem.number = normalizedNumber(FActiveCall->contactJid().uNode());
			histItem.start = FActiveCall->callStartTime().isValid() ? FActiveCall->callStartTime() : QDateTime::currentDateTime();
			histItem.duration = FActiveCall->callDuration();
			prependCallHistory(histItem);
			saveCallHistory();

			FActiveCall = NULL;
			updateDialogState();
		}
	}
}

void PhoneDialerDialog::onSipBalanceRecieved(const Jid &AStreamJid, const ISipBalance &ABalance)
{
	if (AStreamJid == streamJid())
	{
		FBalance = ABalance;
		requestCallCost();
	}
}

void PhoneDialerDialog::onSipCallCostRecieved(const QString &AId, const ISipCallCost &ACost)
{
	if (FCallCostRequestId == AId)
	{
		FCallCost = ACost;
		FCallCostRequestId = QString::null;
		if (FCallCost.cost > -SIP_BALANCE_EPSILON)
		{
			if (FCallCost.cost > FBalance.balance)
			{
				if (FAutoStartCall)
					WidgetManager::showActivateRaiseWindow(window());
				showErrorBalloon(tr("Insufficient funds in the account")+"\n"+tr("Fill up the balance"));
			}
			else if (FAutoStartCall && isCallEnabled())
			{
				QTimer::singleShot(0,this,SLOT(onCallButtonClicked()));
			}
			ui.lneNumber->setText(formattedNumber(ACost.number));
			FAutoStartCall = false;
		}
		updateDialogState();
	}
}
