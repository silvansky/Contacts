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
	virtual bool isContactJidReady() const =0;
	virtual QString gateDescriptorId() const =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QString contactText() const =0;
	virtual void setContactText(const QString &AText) =0;
	virtual Jid gatewayJid() const =0;
	virtual void setGatewayJid(const Jid &AGatewayJid) =0;
	virtual QString errorMessage() const =0;
	virtual void setErrorMessage(const QString &AMessage, bool AInvalidInput, bool AClickable = false) =0;
	virtual bool isServiceIconVisible() const =0;
	virtual void setServiceIconVisible(bool AVisible) =0;
	virtual bool isCloseButtonVisible() const =0;
	virtual void setCloseButtonVisible(bool AVisible) =0;
	virtual bool isErrorClickable() const =0;
	virtual void setErrorClickable(bool AClickable) =0;
	virtual void setCorrectSizes(int ANameSize, int APhotoSize) =0;
protected:
	virtual void adjustSizeRequested() =0;
	virtual void deleteButtonClicked() =0;
	virtual void errorMessageClicked() = 0;
	virtual void contactJidChanged() =0;
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
	virtual QString parentMetaContactId() const =0;
	virtual void setParentMetaContactId(const QString &AMetaId) =0;
	virtual void executeRequiredContactChecks() = 0;
protected:
	virtual void dialogDestroyed() =0;
};

class IRosterChanger
{
public:
	virtual QObject *instance() =0;
	virtual bool isWelcomeScreenVisible() const =0;
	virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isSilentSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isSilentUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual void insertAutoSubscribtion(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr) =0;
	virtual void removeAutoSubscribtion(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = QString::null, bool ASilently = false) =0;
	virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = QString::null, bool ASilently = false) =0;
	virtual IAddMetaItemWidget *newAddMetaItemWidget(const Jid &AStreamJid, const QString &AGateDescriptorId, QWidget *AParent) =0;
	virtual QWidget *showAddContactDialog(const Jid &AStreamJid) =0;
protected:
	virtual void addMetaItemWidgetCreated(IAddMetaItemWidget *AWidget) =0;
	virtual void addContactDialogCreated(IAddContactDialog *ADialog) =0;
	virtual void welcomeScreenVisibleChanged(bool AVisible) =0;
};

Q_DECLARE_INTERFACE(IAddMetaItemWidget,"Virtus.Plugin.IAddMetaItemWidget/1.0")
Q_DECLARE_INTERFACE(IAddContactDialog,"Virtus.Plugin.IAddContactDialog/1.0")
Q_DECLARE_INTERFACE(IRosterChanger,"Virtus.Plugin.IRosterChanger/1.0")

#endif
