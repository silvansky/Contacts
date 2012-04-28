#ifndef CALLCONTROLWIDGET_H
#define CALLCONTROLWIDGET_H

#ifdef USE_PHONON
# include <Phonon/Phonon>
# include <Phonon/MediaSource>
#else
# include <QSound>
#endif

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
	void playSound(const QString &ASoundKey, int ALoops = 0);
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onCallStateChanged(int AState);
	void onCallDeviceStateChanged(int AType, int AState);
	void onCallDevicePropertyChanged(int AType, int AProperty, const QVariant &AValue);
protected slots:
	void onAcceptButtonClicked();
	void onRejectButtonClicked();
	void onSilentButtonClicked();
	void onLocalCameraStateButtonClicked(bool AChecked);
	void onLocalMicrophoneStateButtonClicked(bool AChecked);
	void onRemoteMicrophoneVolumeChanged(qreal AVolume);
protected slots:
	void onCallTimerTimeout();
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
	QTimer FCallTimer;
	ISipCall *FSipCall;
private:
#ifdef USE_PHONON
	Phonon::MediaObject *FMediaObject;
	Phonon::AudioOutput *FAudioOutput;
#else
	QSound *FSound;
#endif
};

#endif // CALLCONTROLWIDGET_H
