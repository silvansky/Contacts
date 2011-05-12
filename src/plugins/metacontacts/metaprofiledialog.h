#ifndef METAPROFILEDIALOG_H
#define METAPROFILEDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <definitions/graphicseffects.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/igateways.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <utils/stylestorage.h>
#include <utils/custombordercontainer.h>
#include <utils/customborderstorage.h>
#include <utils/graphicseffectsstorage.h>
#include <utils/imagemanager.h>
#include "ui_metaprofiledialog.h"

class MetaProfileDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	MetaProfileDialog(IPluginManager *APluginManager, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent = NULL);
	~MetaProfileDialog();
	Jid streamJid() const;
	QString metaContactId() const;
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onMetaAvatarChanged(const QString &AMetaId);
	void onMetaPresenceChanged(const QString &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
private:
	Ui::MetaProfileDialogClass ui;
private:
	IGateways *FGateways;
	IMetaRoster *FMetaRoster;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IRosterChanger *FRosterChanger;
	CustomBorderContainer *FBorder;
private:
	QString FMetaId;
};

#endif // METAPROFILEDIALOG_H
