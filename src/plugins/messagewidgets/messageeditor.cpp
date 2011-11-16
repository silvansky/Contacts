#include "messageeditor.h"

#include <QMimeData>
#include <QTextDocumentFragment>

MessageEditor::MessageEditor(QWidget* parent): AutoSizeTextEdit(parent)
{
	FFormatEnabled = false;
	setUndoRedoEnabled(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

MessageEditor::~MessageEditor()
{

}

bool MessageEditor::isTextFormatEnabled() const
{
	return FFormatEnabled;
}

void MessageEditor::setTextFormatEnabled(bool AEnabled)
{
	FFormatEnabled = AEnabled;
}

bool MessageEditor::canInsertFromMimeData(const QMimeData *ASource) const
{
	if (!isTextFormatEnabled())
		return ASource->hasText() && !ASource->text().isEmpty();
	return AutoSizeTextEdit::canInsertFromMimeData(ASource);
}

void MessageEditor::insertFromMimeData(const QMimeData *ASource)
{
	if (!isTextFormatEnabled())
	{
		textCursor().insertFragment(QTextDocumentFragment::fromPlainText(ASource->text()));
		ensureCursorVisible();
	}
	else
	{
		AutoSizeTextEdit::insertFromMimeData(ASource);
	}
}
