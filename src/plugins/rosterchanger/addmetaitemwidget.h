#ifndef ADDMETAITEMWIDGET_H
#define ADDMETAITEMWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QRadioButton>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionnodes.h>
#include <definitions/gateserviceidentifiers.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/igateways.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>
#include "selectprofilewidget.h"
#include "ui_addmetaitemwidget.h"

class AddMetaItemWidget : 
	public QWidget,
	public IAddMetaItemWidget
{
	Q_OBJECT;
	Q_INTERFACES(IAddMetaItemWidget);
public:
	AddMetaItemWidget(IOptionsManager *AOptionsManager, IRoster *ARoster, IGateways *AGateways, const IGateServiceDescriptor &ADescriptor, QWidget *AParent = NULL);
	~AddMetaItemWidget();
	virtual QWidget *instance() { return this; }
	virtual QString gateDescriptorId() const;
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QString contactText() const;
	virtual void setContactText(const QString &AText);
	virtual Jid gatewayJid() const;
	virtual void setGatewayJid(const Jid &AGatewayJid);
	virtual QString errorMessage() const;
	virtual void setErrorMessage(const QString &AMessage, bool AInvalidInput);
	virtual bool isServiceIconVisible() const;
	virtual void setServiceIconVisible(bool AVisible);
	virtual bool isCloseButtonVisible() const;
	virtual void setCloseButtonVisible(bool AVisible);
	virtual void setCorrectSizes(int ANameSize, int APhotoSize);
signals:
	void adjustSizeRequested();
	void deleteButtonClicked();
	void contactJidChanged(const Jid &AContactJid);
protected:
	QString placeholderTextForGate() const;
protected:
	void startResolve(int ATimeout);
	void setRealContactJid(const Jid &AContactJid);
protected slots:
	void resolveContactJid();
protected slots:
	void onProfilesChanged();
	void onSelectedProfileChanched();
	void onContactTextEditingFinished();
	void onContactTextEdited(const QString &AText);
	void onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
private:
	Ui::AddMetaItemWidgetClass ui;
private:
	IRoster *FRoster;
	IGateways *FGateways;
private:
	QString FContactJidRequest;
private:
	Jid FContactJid;
	QTimer FResolveTimer;
	bool FServiceFailed;
	bool FContactTextChanged;
	IGateServiceDescriptor FDescriptor;
	SelectProfileWidget *FSelectProfileWidget;
};

#endif // ADDMETAITEMWIDGET_H
