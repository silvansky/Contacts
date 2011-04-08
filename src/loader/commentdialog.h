#ifndef COMMENTDIALOG_H
#define COMMENTDIALOG_H

#include <QDialog>
#include <definitions/version.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ivcard.h>
#include <definitions/vcardvaluenames.h>
#include "ui_commentdialog.h"

class CommentDialog : public QDialog
{
    Q_OBJECT

public:
    CommentDialog(IPluginManager *APluginManager, QWidget *AParent = NULL);
    ~CommentDialog();

protected slots:
	//void stanzaSent(const Jid &AStreamJid, const Stanza &AStanza);
	void SendComment();
	void onJidChanded(QString);

private:
	IStanzaProcessor * FStanzaProcessor;
	IMessageProcessor * FMessageProcessor;
	Jid streamJid;
	QString fullName;

private:
    Ui::CommentDialogClass ui;
};

#endif // COMMENTDIALOG_H
