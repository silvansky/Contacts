#ifndef MASSSENDDIALOG_H
#define MASSSENDDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <utils/jid.h>
#include <utils/stylestorage.h>

namespace Ui
{
	class MassSendDialog;
}

class MassSendDialog : 
	public QDialog,
	public IMassSendDialog
{
	Q_OBJECT
	Q_INTERFACES(IMassSendDialog);
public:
	explicit MassSendDialog(IMessageWidgets *AMessageWidgets, const Jid & AStreamJid, QWidget *parent = 0);
	~MassSendDialog();
	virtual QDialog *instance() { return this; }
	// IMassSendDialog
	virtual const Jid &streamJid() const;
	virtual IViewWidget *viewWidget() const;
	virtual IEditWidget *editWidget() const;
	virtual IReceiversWidget *receiversWidget() const;
signals:
	// IMassSendDialog
	void messageReady();
protected slots:
	void onMessageReady();
private:
	Ui::MassSendDialog *ui;
private:
	Jid FStreamJid;
	IViewWidget * FViewWidget;
	IEditWidget * FEditWidget;
	IReceiversWidget * FReceiversWidget;
	IMessageWidgets * FMessageWidgets;
};

#endif // MASSSENDDIALOG_H
