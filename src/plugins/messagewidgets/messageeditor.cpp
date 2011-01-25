#include "messageeditor.h"

MessageEditor::MessageEditor(QWidget* parent): AutoSizeTextEdit(parent)
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

MessageEditor::~MessageEditor()
{
}
