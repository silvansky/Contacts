#include "addcontactdialog.h"

#include <QSet>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>

#define GROUP_NEW         ":group_new:"
#define GROUP_EMPTY       ":empty_group:"

AddContactDialog::AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Add contact"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

	FRoster = NULL;
	FVcardPlugin = NULL;
	FRosterChanger = ARosterChanger;
	FStreamJid = AStreamJid;

	initialize(APluginManager);
	initGroups();
	initGateways();

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add contact"));
}

AddContactDialog::~AddContactDialog()
{
	emit dialogDestroyed();
}

Jid AddContactDialog::streamJid() const
{
	return FStreamJid;
}

Jid AddContactDialog::contactJid() const
{
	return ui.lneContact->text();
}

void AddContactDialog::setContactJid(const Jid &AContactJid)
{
	ui.lneContact->setText(AContactJid.bare());
}

QString AddContactDialog::nickName() const
{
	return ui.lneNick->text();
}

void AddContactDialog::setNickName(const QString &ANick)
{
	ui.lneNick->setText(ANick);
}

QString AddContactDialog::group() const
{
	return ui.cmbGroup->itemData(ui.cmbGroup->currentIndex()).isNull() ? ui.cmbGroup->currentText() : QString::null;
}

void AddContactDialog::setGroup(const QString &AGroup)
{
	ui.cmbGroup->setEditText(AGroup);
}

Jid AddContactDialog::gatewayJid() const
{
	return ui.cmbProfile->itemData(ui.cmbProfile->currentIndex()).toString();
}

void AddContactDialog::setGatewayJid(const Jid &AGatewayJid)
{

}

void AddContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		FRoster = rosterPlugin!=NULL ? rosterPlugin->getRoster(FStreamJid) : NULL;
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVcardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
		if (FGateways)
		{
			connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),SLOT(onServiceLoginReceived(const QString &, const QString &)));
			connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),SLOT(onServiceErrorReceived(const QString &, const QString &)));
		}
	}
}

void AddContactDialog::initGroups()
{
	QList<QString> groups = FRoster!=NULL ? FRoster->groups().toList() : QList<QString>();
	qSort(groups);
	ui.cmbGroup->addItem(tr("<Common Group>"),QString(GROUP_EMPTY));
	ui.cmbGroup->addItems(groups);
	ui.cmbGroup->addItem(tr("New Group..."),QString(GROUP_NEW));

	int last = ui.cmbGroup->findText(Options::node(OPV_ROSTER_ADDCONTACTDIALOG_LASTGROUP).value().toString());
	if (last>=0 && last<ui.cmbGroup->count()-1)
		ui.cmbGroup->setCurrentIndex(last);
	connect(ui.cmbGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
}

void AddContactDialog::initGateways()
{
	if (FGateways)
	{
		ui.cmbProfile->clear();
		ui.cmbProfile->addItem(FStreamJid.bare());

		IDiscoIdentity identity;
		identity.category = "gateway";
		QList<Jid> services = FGateways->streamServices(FStreamJid,identity);
		foreach(Jid serviceJid, services)
		{
			QString id = FGateways->sendLoginRequest(FStreamJid,serviceJid);
			if (!id.isEmpty())
				FLoginRequests.insert(id,serviceJid);
		}
	}	
}

void AddContactDialog::onDialogAccepted()
{
	if (contactJid().isValid())
	{
		if (!FRoster->rosterItem(contactJid()).isValid)
		{
			QSet<QString> groups;
			if (!group().isEmpty())
				groups += group();
			FRoster->setItem(contactJid().bare(),nickName(),groups);
			FRosterChanger->subscribeContact(FStreamJid,contactJid(),QString::null);
			accept();
		}
		else
		{
			QMessageBox::information(NULL,FStreamJid.full(),tr("Contact <b>%1</b> already exists.").arg(contactJid().hBare()));
		}
	}
	else if (!contactJid().isEmpty())
	{
		QMessageBox::warning(this,FStreamJid.bare(),tr("Can`t add contact '<b>%1</b>' because it is not a valid Jaber ID").arg(contactJid().hBare()));
	}
}

void AddContactDialog::onGroupCurrentIndexChanged(int AIndex)
{
	if (ui.cmbGroup->itemData(AIndex).toString() == GROUP_NEW)
	{
		int index = 0;
		QString newGroup = QInputDialog::getText(this,tr("Create group"),tr("New group name")).trimmed();
		if (!newGroup.isEmpty())
		{
			index = ui.cmbGroup->findText(newGroup);
			if (index < 0)
			{
				ui.cmbGroup->blockSignals(true);
				ui.cmbGroup->insertItem(1,newGroup);
				ui.cmbGroup->blockSignals(false);
				index = 1;
			}
			else if (!ui.cmbGroup->itemData(AIndex).isNull())
			{
				index = 0;
			}
		}
		ui.cmbGroup->setCurrentIndex(index);
	}
}

void AddContactDialog::onVCardReceived(const Jid &AContactJid)
{
	Q_UNUSED(AContactJid);
}

void AddContactDialog::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		ui.cmbProfile->addItem(ALogin,serviceJid.full());
	}
}

void AddContactDialog::onServiceErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	FLoginRequests.remove(AId);
}
