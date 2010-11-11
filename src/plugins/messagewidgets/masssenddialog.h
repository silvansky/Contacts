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

class MassSendDialog : public QDialog, public IMassSendDialog
{
	Q_OBJECT
	Q_INTERFACES(IMassSendDialog ITabPage);
public:
	explicit MassSendDialog(IMessageWidgets *AMessageWidgets, const Jid & AStreamJid, QWidget *parent = 0);
	~MassSendDialog();
	// ITabPage
	virtual QDialog *instance() { return this; }
	virtual void showTabPage();
	virtual void closeTabPage();
	virtual QString tabPageId() const;
	virtual bool isActive() const;
	virtual ITabPageNotifier *tabPageNotifier() const
	{
		return FTabPageNotifier;
	}
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier)
	{
		if (FTabPageNotifier != ANotifier)
		{
			if (FTabPageNotifier)
				delete FTabPageNotifier->instance();
			FTabPageNotifier = ANotifier;
			emit tabPageNotifierChanged();
		}
	}
	// IMassSendDialog
	virtual const Jid &streamJid() const
	{
		return FStreamJid;
	}
	virtual IViewWidget *viewWidget() const
	{
		return FViewWidget;
	}
	virtual IEditWidget *editWidget() const
	{
		return FEditWidget;
	}
	virtual IReceiversWidget *receiversWidget() const
	{
		return FReceiversWidget;
	}
signals:
	// ITabPage
	void tabPageShow();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
	// IMassSendDialog
	void messageReady();
protected:
	void changeEvent(QEvent *e);
protected slots:
	void onMessageReady();
private:
	Ui::MassSendDialog *ui;
private:
	Jid FStreamJid;
	IViewWidget * FViewWidget;
	IEditWidget * FEditWidget;
	IReceiversWidget * FReceiversWidget;
	ITabPageNotifier * FTabPageNotifier;
	IMessageWidgets * FMessageWidgets;
};

#endif // MASSSENDDIALOG_H
