#include "messageeditor.h"

#include <QMimeData>
#include <QTextBlock>
#include <QTextCursor>
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

void MessageEditor::insertFromMimeData(const QMimeData *ASource)
{
	if (!isTextFormatEnabled() && acceptRichText())
	{
		QTextDocument doc;
		
		QTextCursor cursor(&doc);
		QTextDocumentFragment fragment;
		if (ASource->hasHtml() && acceptRichText())
			fragment = QTextDocumentFragment::fromHtml(ASource->html(),&doc);
		else
			fragment = QTextDocumentFragment::fromPlainText(ASource->text());
		cursor.insertFragment(fragment);

		QTextCharFormat emptyFormat;
		QTextBlock block = doc.firstBlock();
		while (block.isValid())
		{
			for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
			{
				QTextCharFormat textFormat = it.fragment().charFormat();
				if (!textFormat.isImageFormat() && textFormat!=emptyFormat)
				{
					cursor.setPosition(it.fragment().position());
					cursor.setPosition(it.fragment().position() + it.fragment().length(), QTextCursor::KeepAnchor);
					cursor.setCharFormat(emptyFormat);
				}
			}
			block = block.next();
		}

		cursor.select(QTextCursor::Document);
		textCursor().insertFragment(cursor.selection());
		ensureCursorVisible();
	}
	else
	{
		AutoSizeTextEdit::insertFromMimeData(ASource);
	}
	setFocus();
}
