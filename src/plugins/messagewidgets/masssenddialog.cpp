#include "masssenddialog.h"
#include "ui_masssenddialog.h"
#include <utils/widgetmanager.h>

MassSendDialog::MassSendDialog(const Jid & AStreamJid, QWidget *parent) :
		QDialog(parent),
		ui(new Ui::MassSendDialog),
		FStreamJid(AStreamJid)
{
	ui->setupUi(this);
}

MassSendDialog::~MassSendDialog()
{
	delete ui;
}

void MassSendDialog::showTabPage()
{
	if (isWindow())
	{
		isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
		WidgetManager::raiseWidget(this);
	}
	else
		emit tabPageShow();
}

void MassSendDialog::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QString MassSendDialog::tabPageId() const
{
	return "MassSendDialog|"+FStreamJid.pBare();
}

void MassSendDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
