#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QUrl>
#include <QTimer>
#include <QDialog>
#include <QRadioButton>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/stylesheets.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/gateserviceidentifiers.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/igateways.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imessageprocessor.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customlistview.h>
#include <utils/custominputdialog.h>
#include "selectprofilewidget.h"
#include "ui_addcontactdialog.h"

class AddContactDialog :
	public QDialog,
	public IAddContactDialog
{
	Q_OBJECT
	Q_INTERFACES(IAddContactDialog)
public:
	AddContactDialog(IRoster *ARoster, IRosterChanger *ARosterChanger, IPluginManager *APluginManager, QWidget *AParent = NULL);
	~AddContactDialog();
	//IAddContactDialog
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QString contactText() const;
	virtual void setContactText(const QString &AText);
	virtual QString nickName() const;
	virtual void setNickName(const QString &ANick);
	virtual QString group() const;
	virtual void setGroup(const QString &AGroup);
	virtual Jid gatewayJid() const;
	virtual void setGatewayJid(const Jid &AGatewayJid);
	virtual QString parentMetaContactId() const;
	virtual void setParentMetaContactId(const QString &AMetaId);
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
	void initGroups();
protected:
	void selectRosterIndex();
	QString defaultContactNick(const Jid &AContactJid) const;
	QString confirmDescriptorText(const IGateServiceDescriptor &ADescriptor);
	int registerDescriptorStatus(const IGateServiceDescriptor &ADescriptor);
protected:
	void updatePageAddress();
	void updatePageConfirm(const QList<IGateServiceDescriptor> &ADescriptors);
	void updatePageParams(const IGateServiceDescriptor &ADescriptor);
protected:
	void setDialogState(int AState);
	void setDialogEnabled(bool AEnabled);
	void setRealContactJid(const Jid &AContactJid);
	void setResolveNickState(bool AResole);
	void setErrorMessage(const QString &AMessage, bool AInvalidInput);
protected:
	void resolveDescriptor();
	void resolveContactJid();
	void resolveContactName();
	void resolveLinkedContactsJid();
protected:
	virtual void showEvent(QShowEvent *AEvent);
	void mousePressEvent(QMouseEvent *);
	void resizeEvent(QResizeEvent *);
	void moveEvent(QMoveEvent *);
	bool event(QEvent *);
	bool eventFilter(QObject *, QEvent *);
protected slots:
	void onBackButtonclicked();
	void onContinueButtonclicked();
	void onCancelButtonclicked();
	void onAdjustDialogSize();
	void onContactTextEdited(const QString &AText);
	void onContactNickEdited(const QString &AText);
	void onGroupCurrentIndexChanged(int AIndex);
	void onNewGroupNameSelected(const QString &AGroup);
	void onSelectedProfileChanched();
	void onVCardReceived(const Jid &AContactJid);
	void onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void onBorderReszeMove();
private:
	Ui::AddContactDialogClass ui;
private:
	IRoster *FRoster;
	IAvatars *FAvatars;
	IGateways *FGateways;
	IMetaRoster *FMetaRoster;
	IRostersView *FRostersView;
	IVCardPlugin *FVcardPlugin;
	IRosterChanger *FRosterChanger;
	IOptionsManager *FOptionsManager;
	IMessageProcessor *FMessageProcessor;
private:
	QString FContactJidRequest;
	QString FContactCreateRequest;
	QMap<QString, Jid> FLinkedJidRequests;
private:
	bool FShown;
	Jid FContactJid;
	int FDialogState;
	bool FResolveNick;
	bool FServiceFailed;
	QString FParentMetaId;
	QList<Jid> FLinkedContacts;
	IGateServiceDescriptor FDescriptor;
	SelectProfileWidget *FSelectProfileWidget;
	QMap<QRadioButton *, IGateServiceDescriptor> FConfirmButtons;
};

#endif // ADDCONTACTDIALOG_H
