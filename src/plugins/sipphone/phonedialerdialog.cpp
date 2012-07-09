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
#include <definitions/optionvalues.h>
#include <definitions/customborder.h>
#include <definitions/gateserviceidentifiers.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/balloontip.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>

#define MAX_HISTORY_ROWS  8

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

	FAutoStartCall = false;

	FGateways = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = NULL;
	initialize(APluginManager);

	window()->setWindowTitle(tr("Calls"));

	FNumberMapper.setMapping(ui.pbtNumber_1,"1");
	connect(ui.pbtNumber_1,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_2,"2");
	connect(ui.pbtNumber_2,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_3,"3");
	connect(ui.pbtNumber_3,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_4,"4");
	connect(ui.pbtNumber_4,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_5,"5");
	connect(ui.pbtNumber_5,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_6,"6");
	connect(ui.pbtNumber_6,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_7,"7");
	connect(ui.pbtNumber_7,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_8,"8");
	connect(ui.pbtNumber_8,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_9,"9");
	connect(ui.pbtNumber_9,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_10,"*");
	connect(ui.pbtNumber_10,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_11,"0");
	connect(ui.pbtNumber_11,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	FNumberMapper.setMapping(ui.pbtNumber_12,"#");
	connect(ui.pbtNumber_12,SIGNAL(clicked()),&FNumberMapper,SLOT(map()));
	connect(&FNumberMapper,SIGNAL(mapped(const QString &)),SLOT(onNumberButtonMapped(const QString &)));

	ui.lblCity->setElideMode(Qt::ElideRight);
	ui.lblCost->setElideMode(Qt::ElideRight);
	ui.lblBalance->setElideMode(Qt::ElideRight);
	ui.lblFullHistory->setElideMode(Qt::ElideRight);
	ui.lblFullHistory->setText(QString("<a href='http://contacts.rambler.ru'><span style='text-decoration: underline; color:%1;'>%2</span></a>")
		.arg(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_DARK_LINK_COLOR).name(), tr("Complete call history")));

	//QRegExp phoneNumber("\\+?[\\d|\\*|\\#]+");
	//ui.lneNumber->setValidator(new QRegExpValidator(phoneNumber,ui.lneNumber));

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

	FCostRequestTimer.setSingleShot(true);
	FCostRequestTimer.setInterval(1000);
	connect(&FCostRequestTimer,SIGNAL(timeout()),SLOT(onCostRequestTimerTimeout()));

	FLoadHistoryTimer.setSingleShot(true);
	FLoadHistoryTimer.setInterval(200);
	connect(&FLoadHistoryTimer,SIGNAL(timeout()),SLOT(loadCallHistory()));

	connect(ui.pbtCall,SIGNAL(clicked()),SLOT(onCallButtonClicked()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtCall,MNI_SIPPHONE_DIALER_CALL);

	connect(ui.tbwHistory,SIGNAL(cellDoubleClicked(int,int)),SLOT(onHistoryCellDoubleClicked(int,int)));

	connect(FXmppStream->instance(),SIGNAL(opened()),SLOT(onXmppStreamOpened()));
	connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));

	connect(FSipManager->instance(),SIGNAL(sipBalanceRecieved(const Jid &, const ISipBalance &)),SLOT(onSipBalanceRecieved(const Jid &, const ISipBalance &)));
	connect(FSipManager->instance(),SIGNAL(sipCallCostRecieved(const QString &, const ISipCallCost &)),SLOT(onSipCallCostRecieved(const QString &, const ISipCallCost &)));

	connect(ui.lneNumber,SIGNAL(textChanged(const QString &)),SLOT(onNumberTextChanged(const QString &)));

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	loadCallHistory();
	requestBalance();
}

PhoneDialerDialog::~PhoneDialerDialog()
{
	BalloonTip::hideBalloon();
}

Jid PhoneDialerDialog::streamJid() const
{
	return FXmppStream->streamJid();
}

void PhoneDialerDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IGateways").value(0);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
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
}

void PhoneDialerDialog::requestBalance()
{
	if (FXmppStream->isOpen())
	{
		FBalance = ISipBalance();
		if (!FSipManager->requestBalance(streamJid()))
			FBalance.error = ErrorHandler(ErrorHandler::SERVICE_UNAVAILABLE);
		updateDialogState();
	}
}

void PhoneDialerDialog::requestCallCost()
{
	if (FXmppStream->isOpen())
	{
		FAutoStartCall = false;
		FCallCost = ISipCallCost();
		QString number = normalizedNumber(ui.lneNumber->text());
		if (!FBalance.currency.code.isEmpty() && number.length() >= 3)
		{
			FCostRequestTimer.stop();
			FCallCostRequestId = FSipManager->requestCallCost(streamJid(),FBalance.currency.code,number,QDateTime::currentDateTime(),60000);
			if (FCallCostRequestId.isEmpty())
				FCallCost.error = ErrorHandler(ErrorHandler::SERVICE_UNAVAILABLE);
		}
		updateDialogState();
	}
}

void PhoneDialerDialog::updateDialogState()
{
	QString balanceTempl = QString("<a href='http://contacts.rambler.ru'><span style='text-decoration: underline; color:%1;'>%2</span></a>")
		.arg(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_LINK_COLOR).name());

	if (FBalance.balance > -SIP_BALANCE_EPSILON)
		ui.lblBalance->setText(balanceTempl.arg(currncyValue(FBalance.balance,FBalance.currency)+"."));
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
		ui.lblCost->setText(currncyValue(FCallCost.cost,FCallCost.currency)+"/"+tr("min")+".");
	}
	else if (!FCallCostRequestId.isEmpty())
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(tr("Requesting call cost..."));
	}
	else if (costErrCond=="invalid-phone-number" || costErrCond=="incomplete-phone-number")
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(tr("Incorrect number"));
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
}

void PhoneDialerDialog::showErrorBalloon(const QString &AHtml)
{
	if (window()->isActiveWindow())
	{
		QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_DIALER_ERROR);
		
		QPoint point = ui.lneNumber->mapToGlobal(ui.lneNumber->rect().topRight());
		point.setY(point.y() + ui.lneNumber->height() / 2);

		BalloonTip::showBalloon(icon,QString::null,AHtml,point,0,true,BalloonTip::ArrowLeft,window());
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
		return tr("Tomorrow");
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
		itemName->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AItem.callFailed ? MNI_SIPPHONE_CALL_MISSED : MNI_SIPPHONE_CALL_OUT));

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

bool PhoneDialerDialog::isCallEnabled() const
{
	return FBalance.balance>SIP_BALANCE_EPSILON && FCallCost.cost>SIP_BALANCE_EPSILON && FCallCost.cost<=FBalance.balance;
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

QString PhoneDialerDialog::currncyValue(float AValue, const ISipCurrency &ACurrency) const
{
	return QString("%1 %2").arg(AValue,0,'f',qMax(-ACurrency.exp,0)).arg(ACurrency.code);
}

void PhoneDialerDialog::saveCallHistory()
{
	Options::node(OPV_SIPPHONE_DIALER_HISTORY_ROOT).removeChilds();
	for (int row=0; row<ui.tbwHistory->rowCount(); row++)
	{
		QString ns = QString::number(row);
		CallHistoryItem historyItem = FCallHistory.value(ui.tbwHistory->item(row,HTC_NAME));
		Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).setValue(historyItem.number,"number");
		Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).setValue(DateTime(historyItem.start).toX85DateTime(),"start");
		Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).setValue(historyItem.duration,"duration");
		Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).setValue(historyItem.callFailed,"failed");
	}
	FLoadHistoryTimer.stop();
}

void PhoneDialerDialog::loadCallHistory()
{
	ui.tbwHistory->clear();
	ui.tbwHistory->setColumnCount(HTC_COUNT);
	ui.tbwHistory->horizontalHeader()->setResizeMode(HTC_NAME,QHeaderView::Stretch);
	ui.tbwHistory->horizontalHeader()->setResizeMode(HTC_START,QHeaderView::ResizeToContents);

	for (int row=MAX_HISTORY_ROWS; row>=0; row--)
	{
		QString ns = QString::number(row);
		CallHistoryItem historyItem;
		historyItem.number = Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).value("number").toString();
		historyItem.start = DateTime(Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).value("start").toString()).toLocal();
		historyItem.duration = Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).value("duration").toInt();
		historyItem.callFailed = Options::node(OPV_SIPPHONE_DIALER_HISTORY_CALL_ITEM,ns).value("failed").toBool();
		prependCallHistory(historyItem);
	}
}

void PhoneDialerDialog::onXmppStreamOpened()
{
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
		ISipCall *call = FSipManager->newCall(streamJid(),phoneJid);
		if (call)
		{
			QWidget *callWindow = FSipManager->showCallWindow(call);
			connect(callWindow,SIGNAL(destroyed()),SLOT(onCallWindowDestroyed()));
			connect(call->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
			call->startCall();
			window()->hide();
		}
	}
}

void PhoneDialerDialog::onCostRequestTimerTimeout()
{
	requestCallCost();
}

void PhoneDialerDialog::onNumberButtonMapped(const QString &AText)
{
	if (ui.lneNumber->hasSelectedText())
		ui.lneNumber->del();
	ui.lneNumber->insert(AText);
	ui.lneNumber->setFocus();
}

void PhoneDialerDialog::onNumberTextChanged(const QString &AText)
{
	QString phone = normalizedNumber(AText);
	if (FCallCost.number != phone)
	{
		FCallCost = ISipCallCost();
		updateDialogState();
		FCostRequestTimer.start();
		BalloonTip::hideBalloon();
	}
}

void PhoneDialerDialog::onOptionsChanged(const OptionsNode &ANode)
{
	if (Options::node(OPV_SIPPHONE_DIALER_HISTORY_ROOT).isChildNode(ANode))
	{
		FLoadHistoryTimer.start();
	}
}

void PhoneDialerDialog::onHistoryCellDoubleClicked(int ARow, int AColumn)
{
	Q_UNUSED(AColumn);
	CallHistoryItem historyItem = FCallHistory.value(ui.tbwHistory->item(ARow,HTC_NAME));
	if (!historyItem.number.isEmpty())
	{
		ui.lneNumber->setText(historyItem.number);
		requestCallCost();
		FAutoStartCall = true;
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
		ISipCall *call = qobject_cast<ISipCall *>(sender());
		if (call)
		{
			CallHistoryItem histItem;
			histItem.callFailed = AState==ISipCall::CS_ERROR;
			histItem.number = normalizedNumber(call->contactJid().uNode());
			histItem.start = !histItem.callFailed ? call->callStartTime() : QDateTime::currentDateTime();
			histItem.duration = call->callDuration();
			prependCallHistory(histItem);
			saveCallHistory();
		}
		if (AState == ISipCall::CS_FINISHED)
			ui.lneNumber->clear();
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
				showErrorBalloon(tr("Insufficient funds in the account")+"\n"+tr("Fill up the balance"));
			else if (FAutoStartCall)
				QTimer::singleShot(0,this,SLOT(onCallButtonClicked()));
			ui.lneNumber->setText(formattedNumber(ACost.number));
		}
		updateDialogState();
	}
}
