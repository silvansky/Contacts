#include "messengeroptions.h"

MessengerOptions::MessengerOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	connect(ui.rdbSendByEnter,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbSendByCtrlEnter,SIGNAL(toggled(bool)),SIGNAL(modified()));

#ifdef Q_WS_MAC
	ui.rdbSendByEnter->setText(tr("By pressing ") + QChar(0x23CE)); // Enter symbol
	ui.rdbSendByCtrlEnter->setText(tr("By pressing ") + QChar(0x2318) + QChar(0x23CE)); // Cmd and Enter symbols
#endif
	
	reset();
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::apply()
{
	if (ui.rdbSendByEnter->isChecked())
		Options::node(OPV_MESSAGES_EDITORSENDKEY).setValue(QKeySequence(Qt::Key_Return));
	else
		Options::node(OPV_MESSAGES_EDITORSENDKEY).setValue(QKeySequence(Qt::CTRL | Qt::Key_Return));
	emit childApply();
}

void MessengerOptions::reset()
{
	if (Options::node(OPV_MESSAGES_EDITORSENDKEY).value().value<QKeySequence>() == QKeySequence(Qt::Key_Return))
		ui.rdbSendByEnter->setChecked(true);
	else
		ui.rdbSendByCtrlEnter->setChecked(true);
	emit childReset();
}
