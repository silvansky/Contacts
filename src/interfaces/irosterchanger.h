#ifndef IROSTERCHANGER_H
#define IROSTERCHANGER_H

#include <QDialog>
#include <QVBoxLayout>
#include <utils/jid.h>
#include <utils/toolbarchanger.h>

#define ROSTERCHANGER_UUID "{8d7070c2-46db-4ad1-ac51-eaeed11408fc}"

class IAddMetaItemWidget
{
public:
	virtual QWidget *instance() =0;
	virtual QString gateDescriptorId() const =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QString contactText() const =0;
	virtual void setContactText(const QString &AText) =0;
	virtual Jid gatewayJid() const =0;
	virtual void setGatewayJid(const Jid &AGatewayJid) =0;
	virtual QString errorMessage() const =0;
	virtual void setErrorMessage(const QString &AMessage, bool AInvalidInput) =0;
	virtual bool isServiceIconVisible() const =0;
	virtual void setServiceIconVisible(bool AVisible) =0;
	virtual bool isCloseButtonVisible() const =0;
	virtual void setCloseButtonVisible(bool AVisible) =0;
	virtual void setCorrectSizes(int ANameSize, int APhotoSize) =0;
protected:
	virtual void adjustSizeRequested() =0;
	virtual void deleteButtonClicked() =0;
	virtual void contactJidChanged(const Jid &AContactJid) =0;
};

class IAddContactDialog
{
public:
	virtual QDialog *instance() =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QString contactText() const =0;
	virtual void setContactText(const QString &AText) =0;
	virtual QString nickName() const =0;
	virtual void setNickName(const QString &ANick) =0;
	virtual QString group() const =0;
	virtual void setGroup(const QString &AGroup) =0;
	virtual Jid gatewayJid() const =0;
	virtual void setGatewayJid(const Jid &AGatewayJid) =0;
protected:
	virtual void dialogDestroyed() =0;
};

class ISubscriptionDialog
{
public:
	virtual QDialog *instance() =0;
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual QVBoxLayout *actionsLayout() const =0;
	virtual ToolBarChanger *toolBarChanger() const =0;
protected:
	virtual void dialogDestroyed() =0;
};

class IRosterChanger
{
public:
	virtual QObject *instance() =0;
	virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual void insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr) =0;
	virtual void removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false) =0;
	virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false) =0;
	virtual IAddMetaItemWidget *newAddMetaItemWidget(const Jid &AStreamJid, const QString &AGateDescriptorId, QWidget *AParent) =0;
	virtual QWidget *showAddContactDialog(const Jid &AStreamJid) =0;
protected:
	virtual void addMetaItemWidgetCreated(IAddMetaItemWidget *AWidget) =0;
	virtual void addContactDialogCreated(IAddContactDialog *ADialog) =0;
	virtual void subscriptionDialogCreated(ISubscriptionDialog *ADialog) =0;
};

Q_DECLARE_INTERFACE(IAddMetaItemWidget,"Virtus.Plugin.IAddMetaItemWidget/1.0")
Q_DECLARE_INTERFACE(IAddContactDialog,"Virtus.Plugin.IAddContactDialog/1.0")
Q_DECLARE_INTERFACE(ISubscriptionDialog,"Virtus.Plugin.ISubscriptionDialog/1.0")
Q_DECLARE_INTERFACE(IRosterChanger,"Virtus.Plugin.IRosterChanger/1.0")

#endif
