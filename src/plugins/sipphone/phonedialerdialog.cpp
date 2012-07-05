#include "phonedialerdialog.h"

#include <QCompleter>
#include <QClipboard>
#include <QApplication>
#include <QRegExpValidator>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylevalues.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <definitions/gateserviceidentifiers.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

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

	FGateways = NULL;
	FRosterPlugin = NULL;
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

	QRegExp phoneNumber("\\+?[\\d|\\*|\\#]+");
	ui.lneNumber->setValidator(new QRegExpValidator(phoneNumber,ui.lneNumber));

	/* Compliter
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid()) : NULL;
	if (FGateways && roster)
	{
		QStringList knownNumbers;
		QList<Jid> phoneServices = 
			FGateways->gateDescriptorServices(streamJid(),FGateways->gateDescriptorById(GSID_SMS),true) + 
			FGateways->gateDescriptorServices(streamJid(),FGateways->gateDescriptorById(GSID_PHONE),true);

		foreach(IRosterItem ritem, roster->rosterItems())
		{
			if (phoneServices.contains(ritem.itemJid.domain()))
				knownNumbers.append(normalizedNumber(ritem.itemJid.uNode()));
		}

		if (!knownNumbers.isEmpty())
		{
			ui.lneNumber->setCompleter(new QCompleter(knownNumbers,ui.lneNumber));
		}
	}
	*/

	FCostRequestTimer.setSingleShot(true);
	FCostRequestTimer.setInterval(1000);
	connect(&FCostRequestTimer,SIGNAL(timeout()),SLOT(onCostRequestTimerTimeout()));

	connect(ui.pbtCall,SIGNAL(clicked()),SLOT(onCallButtonClicked()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtCall,MNI_SIPPHONE_DIALER_CALL);

	connect(FSipManager->instance(),SIGNAL(sipBalanceRecieved(const Jid &, const ISipBalance &)),SLOT(onSipBalanceRecieved(const Jid &, const ISipBalance &)));
	connect(FSipManager->instance(),SIGNAL(sipCallCostRecieved(const QString &, const ISipCallCost &)),SLOT(onSipCallCostRecieved(const QString &, const ISipCallCost &)));

	connect(ui.lneNumber,SIGNAL(textChanged(const QString &)),SLOT(onNumberTextChanged(const QString &)));
	onNumberTextChanged(ui.lneNumber->text());

	updateDialogState();
	requestBalance();
}

PhoneDialerDialog::~PhoneDialerDialog()
{

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
}

bool PhoneDialerDialog::requestBalance()
{
	if (!FSipManager->requestBalance(streamJid()))
	{
		FBalance = ISipBalance();
		updateDialogState();
		return false;
	}
	return true;
}

bool PhoneDialerDialog::requestCallCost()
{
	QString number = normalizedNumber(ui.lneNumber->text());
	if (!FBalance.currency.code.isEmpty() && number.length() >= 3)
	{
		FCallCostRequestId = FSipManager->requestCallCost(streamJid(),FBalance.currency.code,number,QDateTime::currentDateTime(),60000);
		return !FCallCostRequestId.isEmpty();
	}
	return false;
}

void PhoneDialerDialog::updateDialogState()
{
	QString balanceTempl = QString("<a href='http://contacts,rambler.ru'><span style='text-decoration: underline; color:%1;'>%2</span></a>")
		.arg(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_DARK_LINK_COLOR).name());

	if (FBalance.balance > -SIP_BALANCE_EPSILON)
		ui.lblBalance->setText(balanceTempl.arg(currncyValue(FBalance.balance,FBalance.currency)+"."));
	else
		ui.lblBalance->setText(balanceTempl.arg(tr("Balance is not available")));

	if (FCallCost.cost > -SIP_BALANCE_EPSILON)
	{
		ui.lblCity->setText(!FCallCost.country.isEmpty() ? FCallCost.city+"("+FCallCost.country+")" : FCallCost.city);
		ui.lblCost->setText(currncyValue(FCallCost.cost,FCallCost.currency)+"/"+tr("min")+".");
	}
	else
	{
		ui.lblCity->setText(QString::null);
		ui.lblCost->setText(QString::null);
	}

	ui.pbtCall->setEnabled(isCallEnabled());
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

QString PhoneDialerDialog::currncyValue(float AValue, const ISipCurrency &ACurrency)
{
	return QString("%1 %2").arg(AValue,0,'f',qMax(-ACurrency.exp,0)).arg(ACurrency.code);
}

void PhoneDialerDialog::onNumberButtonMapped(const QString &AText)
{
	if (ui.lneNumber->hasSelectedText())
		ui.lneNumber->del();
	ui.lneNumber->insert(AText);
	ui.lneNumber->setFocus();
}

void PhoneDialerDialog::onCallButtonClicked()
{
	if (isCallEnabled())
	{
		QString number = FCallCost.phone;
		if (number.startsWith('+'))
			number.remove(0,1);

		Jid phoneJid(number,SIPPHONE_SERVICE_JID,QString::null);
		ISipCall *call = FSipManager->newCall(streamJid(),phoneJid);
		if (call)
		{
			FSipManager->showCallWindow(call);
			call->startCall();
		}
	}
}

void PhoneDialerDialog::onCostRequestTimerTimeout()
{
	requestCallCost();
}

void PhoneDialerDialog::onNumberTextChanged(const QString &AText)
{
	QString phone = normalizedNumber(ui.lneNumber->text());
	if (FCallCost.phone != phone)
	{
		FCallCost = ISipCallCost();
		FCostRequestTimer.start();
		updateDialogState();
	}
}

void PhoneDialerDialog::onSipBalanceRecieved(const Jid &AStreamJid, const ISipBalance &ABalance)
{
	if (AStreamJid == streamJid())
	{
		FBalance = ABalance;
		updateDialogState();
	}
}

void PhoneDialerDialog::onSipCallCostRecieved(const QString &AId, const ISipCallCost &ACost)
{
	if (FCallCostRequestId == AId)
	{
		FCallCost = ACost;
		FCallCostRequestId = QString::null;
		if (FCallCost.cost > -SIP_BALANCE_EPSILON)
			ui.lneNumber->setText(formattedNumber(ACost.phone));
		updateDialogState();
	}
}
