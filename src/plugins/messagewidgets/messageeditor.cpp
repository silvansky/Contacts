#include "messageeditor.h"

MessageEditor::MessageEditor(QWidget* parent): AutoSizeTextEdit(parent)
{
	setUndoRedoEnabled(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

MessageEditor::~MessageEditor()
{

}
