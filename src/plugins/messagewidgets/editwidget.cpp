#include "editwidget.h"

#include <QPair>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <utils/stylestorage.h>
#include <definitions/resources.h>

#define MAX_BUFFERED_MESSAGES     10

EditWidget::EditWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	ui.medEditor->setAcceptRichText(true);
	ui.medEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	
	QVBoxLayout *vlayout = new QVBoxLayout;
	vlayout->setMargin(1);
	vlayout->addWidget(ui.tlbSend,0,Qt::AlignBottom);

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addStretch();
	hlayout->setContentsMargins(2, 2, 20, 2);
	hlayout->addLayout(vlayout);

	ui.medEditor->setLayout(hlayout);
	ui.medEditor->setAcceptRichText(false);
	ui.medEditor->installEventFilter(this);
	ui.medEditor->setLineWrapMode(QTextEdit::FixedPixelWidth);

	FMessageWidgets = AMessageWidgets;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FBufferPos = -1;

	FSendShortcut = new QShortcut(ui.medEditor);
	FSendShortcut->setContext(Qt::WidgetShortcut);
	connect(FSendShortcut,SIGNAL(activated()),SLOT(onShortcutActivated()));

	connect(ui.tlbSend,SIGNAL(clicked(bool)),SLOT(onSendButtonCliked(bool)));

	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORAUTORESIZE));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORMINIMUMLINES));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORMAXIMUMLINES));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORSENDKEY));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

EditWidget::~EditWidget()
{

}

const Jid &EditWidget::streamJid() const
{
	return FStreamJid;
}

void EditWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid befour = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(befour);
	}
}

const Jid & EditWidget::contactJid() const
{
	return FContactJid;
}

void EditWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid befour = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(befour);
	}
}

QTextEdit *EditWidget::textEdit() const
{
	return ui.medEditor;
}

QTextDocument *EditWidget::document() const
{
	return ui.medEditor->document();
}

void EditWidget::sendMessage()
{
	emit messageAboutToBeSend();
	appendMessageToBuffer();
	emit messageReady();
}

void EditWidget::clearEditor()
{
	ui.medEditor->clear();
	emit editorCleared();
}

bool EditWidget::autoResize() const
{
	return ui.medEditor->autoResize();
}

void EditWidget::setAutoResize(bool AResize)
{
	ui.medEditor->setAutoResize(AResize);
	emit autoResizeChanged(ui.medEditor->autoResize());
}

int EditWidget::minimumLines() const
{
	return ui.medEditor->minimumLines();
}

void EditWidget::setMinimumLines(int ALines)
{
	ui.medEditor->setMinimumLines(ALines);
	emit minimumLinesChanged(ui.medEditor->minimumLines());
}

int EditWidget::maximumLines() const
{
	return ui.medEditor->maximumLines();
}

void EditWidget::setMaximumLines(int ALines)
{
	ui.medEditor->setMaximumLines(ALines);
	emit maximumLinesChanged(ui.medEditor->maximumLines());
}

QKeySequence EditWidget::sendKey() const
{
	return FSendShortcut->key();
}

void EditWidget::setSendKey(const QKeySequence &AKey)
{
	FSendShortcut->setKey(AKey);
	if (!AKey.isEmpty())
		ui.tlbSend->setToolTip(tr("Send message (%1)").arg(AKey.toString().replace("Return","Enter")));
	else
		ui.tlbSend->setToolTip(tr("Send message"));
	emit sendKeyChanged(AKey);
}

bool EditWidget::sendButtonVisible() const
{
	return ui.tlbSend->isVisible();
}

void EditWidget::setSendButtonVisible(bool AVisible)
{
	ui.tlbSend->setVisible(AVisible);
}

bool EditWidget::sendButtonEnabled() const
{
	return ui.tlbSend->isEnabled();
}

void EditWidget::setSendButtonEnabled(bool AEnabled)
{
	ui.tlbSend->setEnabled(AEnabled);
}

bool EditWidget::textFormatEnabled() const
{
	return ui.medEditor->isTextFormatEnabled();
}

void EditWidget::setTextFormatEnabled(bool AEnabled)
{
	ui.medEditor->setTextFormatEnabled(AEnabled);
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	bool hooked = false;
	if (AWatched==ui.medEditor && AEvent->type()==QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		emit keyEventReceived(keyEvent, hooked);

		if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key()==Qt::Key_Up)
		{
			hooked = true;
			showNextBufferedMessage();
		}
		else if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key() == Qt::Key_Down)
		{
			hooked = true;
			showPrevBufferedMessage();
		}
	}
	else if (AWatched==ui.medEditor && AEvent->type()==QEvent::ShortcutOverride)
	{
		hooked = true;
	}
	else if (AWatched==ui.medEditor && AEvent->type()==QEvent::Resize)
	{
		static const int rightMargin = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_MW_EDIT_RIGHT_MARGIN);
		QResizeEvent * resEvent = (QResizeEvent*)AEvent;
		ui.medEditor->setLineWrapColumnOrWidth(resEvent->size().width() - rightMargin);
	}
	return hooked || QWidget::eventFilter(AWatched,AEvent);
}

void EditWidget::appendMessageToBuffer()
{
	QString message = ui.medEditor->toPlainText();
	if (!message.isEmpty())
	{
		FBufferPos = -1;
		int index = FBuffer.indexOf(message);
		if (index >= 0)
			FBuffer.removeAt(index);
		FBuffer.prepend(message);
		if (FBuffer.count() > MAX_BUFFERED_MESSAGES)
			FBuffer.removeLast();
	}
}

void EditWidget::showBufferedMessage()
{
	ui.medEditor->setPlainText(FBuffer.value(FBufferPos));
}

void EditWidget::showNextBufferedMessage()
{
	if (FBufferPos < FBuffer.count()-1)
	{
		if (FBufferPos<0 && !ui.medEditor->toPlainText().isEmpty())
		{
			appendMessageToBuffer();
			FBufferPos++;
		}
		FBufferPos++;
		showBufferedMessage();
	}
}

void EditWidget::showPrevBufferedMessage()
{
	if (FBufferPos > 0)
	{
		FBufferPos--;
		showBufferedMessage();
	}
}

void EditWidget::onShortcutActivated()
{
	QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
	if (shortcut == FSendShortcut)
	{
		sendMessage();
	}
}

void EditWidget::onSendButtonCliked(bool)
{
	sendMessage();
}

void EditWidget::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_EDITORAUTORESIZE)
	{
		setAutoResize(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_MESSAGES_EDITORMINIMUMLINES)
	{
		setMinimumLines(ANode.value().toInt());
	}
	else if (ANode.path() == OPV_MESSAGES_EDITORMAXIMUMLINES)
	{
		setMaximumLines(ANode.value().toInt());
	}
	else if (ANode.path() == OPV_MESSAGES_EDITORSENDKEY)
	{
		QKeySequence key = ANode.value().value<QKeySequence>();
		if (key.isEmpty())
			key = Options::defaultValue(OPV_MESSAGES_EDITORSENDKEY).value<QKeySequence>();
		setSendKey(key);
		;
	}
}
