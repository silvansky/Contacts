#ifndef SIMPLEVCARDDIALOG_H
#define SIMPLEVCARDDIALOG_H

#include <QDialog>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ivcard.h>
#include <utils/iconstorage.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irosterchanger.h>

namespace Ui
{
	class SimpleVCardDialog;
}

class SimpleVCardDialog : public QDialog
{
	Q_OBJECT
public:
	SimpleVCardDialog(IVCardPlugin *AVCardPlugin, IAvatars *AAvatars, IStatusIcons *AStatusIcons, IRosterPlugin *ARosterPlugin, IPresencePlugin *APresencePlugin, IRosterChanger *ARosterChanger, const Jid &AStreamJid, const Jid &AContactJid);
	~SimpleVCardDialog();
	Jid streamJid() const;
protected:
	void reloadVCard();
	void updateDialog();
protected slots:
	void onVCardUpdated();
	void onVCardError(const QString &AError);
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void on_addToRosterButton_clicked();
	void on_renameButton_clicked();
private:
	Ui::SimpleVCardDialog *ui;
private:
	Jid FContactJid;
	Jid FStreamJid;
	IVCard *FVCard;
	IAvatars *FAvatars;
	IStatusIcons *FStatusIcons;
	IRosterPlugin *FRosterPlugin;
	IRoster *FRoster;
	IRosterItem FRosterItem;
	IPresence *FPresence;
	IRosterChanger *FRosterChanger;
};

#endif // SIMPLEVCARDDIALOG_H
