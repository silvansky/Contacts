#ifndef METAPROFILEDIALOG_H
#define METAPROFILEDIALOG_H

#include <QDialog>
#include <QPointer>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <definitions/graphicseffects.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/metaitemorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/igateways.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ivcard.h>
#include <utils/stylestorage.h>
#include <utils/custombordercontainer.h>
#include <utils/customborderstorage.h>
#include <utils/graphicseffectsstorage.h>
#include <utils/imagemanager.h>
#include <utils/closebutton.h>
#include <utils/widgetmanager.h>
#include <utils/custominputdialog.h>
#include "ui_metaprofiledialog.h"

struct MetaContainer
{
	MetaContainer() {
		metaWidget = NULL;
		itemsWidget = NULL;
	};
	QWidget *metaWidget;
	QLabel *metaLabel;
	QWidget *itemsWidget;
	QMap<Jid, QWidget *> itemWidgets;
};

class MetaProfileDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	MetaProfileDialog(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent = NULL);
	~MetaProfileDialog();
	Jid streamJid() const;
	QString metaContactId() const;
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
	void updateBirthday();
	void updateLeftLabelsSizes();
	QString metaLabelText(const IMetaItemDescriptor &ADescriptor) const;
protected:
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onAdjustDialogSize();
	void onAdjustBorderSize();
	void onContactNameEditFinished();
	void onAddContactButtonClicked();
	void onDeleteContactButtonClicked();
	void onDeleteContactDialogAccepted();
	void onLabelLinkActivated(const QString &ALink);
	void onMetaAvatarChanged(const QString &AMetaId);
	void onMetaPresenceChanged(const QString &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
private:
	Ui::MetaProfileDialogClass ui;
private:
	IGateways *FGateways;
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IRosterChanger *FRosterChanger;
	IVCardPlugin *FVCardPlugin;
	CustomBorderContainer *FBorder;
private:
	QString FMetaId;
	QMap<int, MetaContainer> FMetaContainers;
	QPointer<CustomInputDialog> FDeleteContactDialog;
};

#endif // METAPROFILEDIALOG_H
