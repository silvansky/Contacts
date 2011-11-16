#ifndef MESSAGEEDITOR_H
#define MESSAGEEDITOR_H

#include <QTextEdit>
#include <utils/autosizetextedit.h>

class MessageEditor :
	public AutoSizeTextEdit
{
	Q_OBJECT
public:
	MessageEditor(QWidget* parent);
	~MessageEditor();
	virtual bool isTextFormatEnabled() const;
	virtual void setTextFormatEnabled(bool AEnabled);
protected:
	virtual bool canInsertFromMimeData(const QMimeData *ASource) const;
	virtual void insertFromMimeData(const QMimeData *ASource);
private:
	bool FFormatEnabled;
};

#endif // MESSAGEEDITOR_H
