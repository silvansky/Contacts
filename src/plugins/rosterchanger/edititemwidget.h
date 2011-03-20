#ifndef EDITITEMWIDGET_H
#define EDITITEMWIDGET_H

#include <QWidget>
#include <QRadioButton>
#include <definitions/resources.h>
#include <interfaces/igateways.h>
#include <utils/iconstorage.h>
#include "ui_edititemwidget.h"

class EditItemWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	EditItemWidget(IGateways *AGateways, const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, QWidget *AParent = NULL);
	~EditItemWidget();
signals:
	void deleteButtonClicked();
protected:
	void updateProfiles();
	Jid selectedProfile() const;
	void setSelectedProfile(const Jid &AServiceJid);
protected slots:
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
private:
	Ui::EditItemWidget ui;
private:
	IGateways *FGateways;
private:
	Jid FStreamJid;
	IGateServiceDescriptor FDescriptor;
	QMap<Jid, QRadioButton *> FProfiles;
};

#endif // EDITITEMWIDGET_H
