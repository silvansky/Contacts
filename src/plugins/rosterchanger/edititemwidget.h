#ifndef EDITITEMWIDGET_H
#define EDITITEMWIDGET_H

#include <QTimer>
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
	Jid contactJid() const;
	void setContactJid(const Jid &AContactJid);
	QString contactText() const;
	void setContactText(const QString &AText);
	virtual Jid gatewayJid() const;
	virtual void setGatewayJid(const Jid &AGatewayJid);
	IGateServiceDescriptor gateDescriptor() const;
signals:
	void adjustSizeRequired();
	void deleteButtonClicked();
	void contactJidChanged(const Jid &AContactJid);
protected:
	void updateProfiles();
	Jid selectedProfile() const;
	void setSelectedProfile(const Jid &AServiceJid);
	QString normalContactText(const QString &AText) const;
protected:
	void startResolve(int ATimeout);
	void setRealContactJid(const Jid &AContactJid);
	void setErrorMessage(const QString &AMessage);
protected slots:
	void resolveContactJid();
protected slots:
	void onContactTextEditingFinished();
	void onContactTextEdited(const QString &AText);
	void onProfileButtonClicked(bool);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
private:
	Ui::EditItemWidget ui;
private:
	IGateways *FGateways;
private:
	QString FContactJidRequest;
	QMap<QString, Jid> FLoginRequests;
private:
	Jid FStreamJid;
	Jid FContactJid;
	QTimer FResolveTimer;
	IGateServiceDescriptor FDescriptor;
	QMap<Jid, QRadioButton *> FProfiles;
};

#endif // EDITITEMWIDGET_H
