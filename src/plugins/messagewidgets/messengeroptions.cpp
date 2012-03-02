#include "messengeroptions.h"

MessengerOptions::MessengerOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	connect(ui.rdbSendByEnter,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbSendByCtrlEnter,SIGNAL(toggled(bool)),SIGNAL(modified()));
	ui.rdbSendByEnter->setText(tr("By pressing %1").arg(QKeySequence(Qt::Key_Return).toString(QKeySequence::NativeText).replace("Return","Enter")));
	ui.rdbSendByCtrlEnter->setText(tr("By pressing %1").arg(QKeySequence(Qt::CTRL | Qt::Key_Return).toString(QKeySequence::NativeText).replace("Return","Enter")));

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
	if (Options::node(OPV_MESSAGES_EDITORSENDKEY).value().value<QKeySequence>() == QKeySequence(Qt::CTRL | Qt::Key_Return))
		ui.rdbSendByCtrlEnter->setChecked(true);
	else
		ui.rdbSendByEnter->setChecked(true);
	emit childReset();
}
