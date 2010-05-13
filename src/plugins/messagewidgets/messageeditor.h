#ifndef MESSAGEEDITOR_H
#define MESSAGEEDITOR_H

#include <QTextEdit>
#include <utils/autosizetextedit.h>

class MessageEditor :
			public AutoSizeTextEdit
{
	Q_OBJECT;
public:
	MessageEditor(QWidget* parent);
	~MessageEditor();
};

#endif // MESSAGEEDITOR_H
