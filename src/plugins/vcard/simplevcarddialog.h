#ifndef SIMPLEVCARDDIALOG_H
#define SIMPLEVCARDDIALOG_H

#include <QDialog>
#include <definations/vcardvaluenames.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ivcard.h>
#include <utils/iconstorage.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>

namespace Ui
{
	class SimpleVCardDialog;
}

class SimpleVCardDialog : public QDialog
{
	Q_OBJECT
public:
	SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars, IStatusIcons *AStatusIcons, IRosterPlugin * ARosterPlugin, IPresencePlugin * APresencePlugin, const Jid &AStreamJid, const Jid &AContactJid);
	~SimpleVCardDialog();

protected:
	void changeEvent(QEvent *e);
protected:
	void reloadVCard();
	void updateDialog();
protected slots:
	void onVCardUpdated();
	void onVCardError(const QString &AError);
private:
	Ui::SimpleVCardDialog *ui;
private:
	Jid FContactJid;
	Jid FStreamJid;
	IVCard *FVCard;
	IVCardPlugin * FVCardPlugin;
	IAvatars * FAvatars;
	IStatusIcons * FStatusIcons;
	IRosterPlugin * FRosterPlugin;
	IRoster * FRoster;
	IRosterItem FRosterItem;
	IPresencePlugin * FPresencePlugin;
	IPresence * FPresence;
};

#endif // SIMPLEVCARDDIALOG_H
