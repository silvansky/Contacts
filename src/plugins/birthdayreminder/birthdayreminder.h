#ifndef BIRTHDAYREMINDER_H
#define BIRTHDAYREMINDER_H

#include <QTimer>
#include <QObject>
#include <QPixmap>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/soundfiles.h>
#include <definitions/stylesheets.h>
#include <definitions/optionvalues.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/internalnoticepriorities.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibirthdayreminder.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/iavatars.h>
#include <interfaces/inotifications.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/imessageprocessor.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/iconstorage.h>

class BirthdayReminder : 
	public QObject,
	public IPlugin,
	public IBirthdayReminder,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBirthdayReminder IRosterDataHolder);
public:
	BirthdayReminder();
	~BirthdayReminder();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return BIRTHDAYREMINDER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IBirthdayReminder
	virtual QDate contactBithday(const Jid &AContactJid) const;
	virtual int contactBithdayDaysLeft(const Jid &AContactJid) const;
	virtual QImage avatarWithCake(const Jid &AContactJid, QImage &AAvatar = QImage()) const;
signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	IInternalNotice internalNoticeTemplate() const;
	Jid findContactStream(const Jid &AContactJid) const;
	void updateBirthdaysStates();
	bool updateBirthdayState(const Jid &AContactJid);
	void setContactBithday(const Jid &AContactJid, const QDate &ABirthday);
protected slots:
	void onShowNotificationTimer();
	void onCongratulateWithPostcard();
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onNotificationTest(const QString &ANotificatorId, uchar AKinds);
	void onInternalNoticeReady();
	void onInternalNoticeActionTriggered();
	void onInternalNoticeRemove();
	void onInternalNoticeRemoved(int ANoticeId);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger);
	void onVCardReceived(const Jid &AContactJid);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IAvatars *FAvatars;
	IVCardPlugin *FVCardPlugin;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	INotifications *FNotifications;
	IMainWindowPlugin *FMainWindowPlugin;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageProcessor *FMessageProcessor;
private:
	int FInternalNoticeId;
	QTimer FNotifyTimer;
	QDate FNotifyDate;
	QMap<int, Jid> FNotifies;
	QList<Jid> FNotifiedContacts;
private:
	QPixmap	FAvatarCake;
	QMap<Jid, int> FUpcomingBirthdays;
	QMap<Jid, QDate> FBirthdays;
};

#endif // BIRTHDAYREMINDER_H
