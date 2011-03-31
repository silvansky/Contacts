#ifndef ADDMETACONTACTDIALOG_H
#define ADDMETACONTACTDIALOG_H

#include <QDialog>
#include <QMultiMap>
#include <QVBoxLayout>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/stylesheets.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/optionnodes.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/igateways.h>
#include <interfaces/iavatars.h>
#include <interfaces/ivcard.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/imagemanager.h>
#include "edititemwidget.h"
#include "ui_addmetacontactdialog.h"

class AddMetaContactDialog : 
	public QDialog,
	public IAddContactDialog
{
	Q_OBJECT;
	Q_INTERFACES(IAddContactDialog);
public:
	AddMetaContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
	~AddMetaContactDialog();
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QString contactText() const;
	virtual void setContactText(const QString &AContact);
	virtual QString nickName() const;
	virtual void setNickName(const QString &ANick);
	virtual QString group() const;
	virtual void setGroup(const QString &AGroup);
	virtual Jid gatewayJid() const;
	virtual void setGatewayJid(const Jid &AGatewayJid);
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
	void createGatewaysMenu();
	void resolveClipboardText();
	void addContactItem(const IGateServiceDescriptor &ADescriptor, const QString &AContact = QString::null);
	int descriptorStatus(const IGateServiceDescriptor &ADescriptor) const;
	QString defaultContactNick(const Jid &AContactJid) const;
	void updateDialogState();
	void setDialogEnabled(bool AEnabled);
	void setAvatarIndex(int AIndex);
protected:
	virtual void showEvent(QShowEvent *AEvent);
protected slots:
	void onDialogAccepted();
	void onAdjustDialogSize();
	void onNickResolveTimeout();
	void onPrevPhotoButtonClicked();
	void onNextPhotoButtonClicked();
	void onAddItemActionTriggered(bool);
	void onItemWidgetAdjustSizeRequested();
	void onItemWidgetDeleteButtonClicked();
   void onItemWidgetShowOptionsRequested();
	void onItemWidgetContactJidChanged(const Jid &AContactJid);
	void onVCardReceived(const Jid &AContactJid);
	void onVCardError(const Jid &AContactJid, const QString &AError);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
private:
	QVBoxLayout *FItemsLayout;
	Ui::AddMetaContactDialog ui;
private:
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
	IAvatars *FAvatars;
	IGateways *FGateways;
	IVCardPlugin *FVcardPlugin;
	IRostersView *FRostersView;
	IRosterChanger *FRosterChanger;
   IOptionsManager *FOptionsManager;
private:
	int FAvatarIndex;
	QList<Jid> FValidContacts;
	QList<Jid> FAvatarContacts;
	QList<Jid> FNoVcardContacts;
	QMap<Jid,QImage> FContactAvatars;
private:
	bool FShown;
	bool FNickResolved;
	Jid FStreamJid;
	QString FCreateActiontId;
	QList<QString> FAvailDescriptors;
	QList<EditItemWidget *> FItemWidgets;
};

#endif // ADDMETACONTACTDIALOG_H
