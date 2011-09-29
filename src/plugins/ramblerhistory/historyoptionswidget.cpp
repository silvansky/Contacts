#include "historyoptionswidget.h"

HistoryOptionsWidget::HistoryOptionsWidget(IRamblerHistory *AHistory, IXmppStream *AXmppStream, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FHistory = AHistory;
	FXmppStream = AXmppStream;
	connect(FHistory->instance(),SIGNAL(historyPrefsChanged(const Jid &, const IHistoryStreamPrefs &)),
		SLOT(onHistoryPrefsChanged(const Jid &, const IHistoryStreamPrefs &)));
	connect(ui.chbAutoSave,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	reset();
	onHistoryPrefsChanged(FXmppStream->streamJid(),FHistory->historyPrefs(FXmppStream->streamJid()));
}

HistoryOptionsWidget::~HistoryOptionsWidget()
{

}

void HistoryOptionsWidget::apply()
{
	if (FHistory->isReady(FXmppStream->streamJid()))
	{
		IHistoryStreamPrefs prefs = FHistory->historyPrefs(FXmppStream->streamJid());
		prefs.autoSave = ui.chbAutoSave->isChecked() ? HISTORY_SAVE_TRUE : HISTORY_SAVE_FALSE;
		FHistory->setHistoryPrefs(FXmppStream->streamJid(),prefs);
	}
	emit childApply();
}

void HistoryOptionsWidget::reset()
{
	ui.chbAutoSave->setChecked(FHistory->historyPrefs(FXmppStream->streamJid()).autoSave==HISTORY_SAVE_TRUE ? true : false);
	emit childReset();
}

void HistoryOptionsWidget::onHistoryPrefsChanged(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs)
{
	Q_UNUSED(APrefs);
	if (FXmppStream->streamJid() == AStreamJid)
	{
		if (FHistory->isReady(AStreamJid))
		{
			if (!ui.chbAutoSave->isEnabled())
				reset();
			ui.chbAutoSave->setEnabled(true);
		}
		else
		{
			ui.chbAutoSave->setEnabled(false);
		}
	}
}
