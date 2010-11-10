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
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
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

private slots:
    void on_addToRosterButton_clicked();
    void on_renameButton_clicked();
};

#endif // SIMPLEVCARDDIALOG_H
