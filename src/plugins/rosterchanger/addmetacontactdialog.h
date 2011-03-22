#ifndef ADDMETACONTACTDIALOG_H
#define ADDMETACONTACTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/stylesheets.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/igateways.h>
#include <interfaces/iavatars.h>
#include <interfaces/ivcard.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include "edititemwidget.h"
#include "ui_addmetacontactdialog.h"

class AddMetaContactDialog : 
	public QDialog,
	public IAddMetaContactDialog
{
	Q_OBJECT;
	Q_INTERFACES(IAddMetaContactDialog);
public:
	AddMetaContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
	~AddMetaContactDialog();
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual QString nickName() const;
	virtual void setNickName(const QString &ANick);
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
	void createGatewaysMenu();
	void addContactItem(const IGateServiceDescriptor &ADescriptor);
	int descriptorStatus(const IGateServiceDescriptor &ADescriptor) const;
protected slots:
	void onDialogAccepted();
	void onAdjustDialogSize();
	void onAddItemActionTriggered(bool);
	void onItemWidgetAdjustSizeRequested();
	void onItemWidgetDeleteButtonClicked();
	void onItemWidgetContactJidChanged(const Jid &AContactJid);
private:
	QVBoxLayout *FItemsLayout;
	Ui::AddMetaContactDialog ui;
private:
	IMetaRoster *FMetaRoster;
	IAvatars *FAvatars;
	IGateways *FGateways;
	IVCardPlugin *FVcardPlugin;
	IRosterChanger *FRosterChanger;
private:
	Jid FStreamJid;
	QList<QString> FAvailDescriptors;
	QList<EditItemWidget *> FItemWidgets;
};

#endif // ADDMETACONTACTDIALOG_H
