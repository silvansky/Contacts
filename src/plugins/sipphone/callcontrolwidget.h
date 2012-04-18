#ifndef CALLCONTROLWIDGET_H
#define CALLCONTROLWIDGET_H

#include <QWidget>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isipphone.h>
#include <interfaces/iavatars.h>
#include <interfaces/igateways.h>
#include <interfaces/imetacontacts.h>
#include "ui_callcontrolwidget.h"

class CallControlWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	CallControlWidget(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent = NULL);
	~CallControlWidget();
	Jid streamJid() const;
	Jid contactJid() const;
	ISipCall *sipCall() const;
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onCallStateChanged(int AState);
	void onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState);
protected slots:
	void onAcceptButtonClicked();
	void onRejectButtonClicked();
	void onSilentButtonClicked();
	void onLocalCameraStateButtonClicked(bool AChecked);
	void onLocalMicrophoneStateButtonClicked(bool AChecked);
	void onRemoteMicrophoneStateButtonClicked(bool AChecked);
protected slots:
	void onMetaAvatarChanged(const QString &AMetaId);
private:
	Ui::CallControlWidgetClass ui;
private:
	IAvatars *FAvatars;
	IGateways *FGateways;
	IMetaRoster *FMetaRoster;
	IMetaContacts *FMetaContacts;
private:
	QString FMetaId;
	ISipCall *FSipCall;
};

#endif // CALLCONTROLWIDGET_H
