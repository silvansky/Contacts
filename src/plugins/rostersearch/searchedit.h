#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>

class SearchEdit : 
	public QLineEdit
{
	Q_OBJECT
public:
	enum IconState
	{
		Ready,
		InProgress,
		Hover
	};
	explicit SearchEdit(QWidget *parent = 0);
	void processKeyPressEvent(QKeyEvent * event);
protected:
	virtual void resizeEvent(QResizeEvent *);
	virtual void mouseMoveEvent(QMouseEvent *);
	virtual void mousePressEvent(QMouseEvent *);
	virtual void leaveEvent(QEvent *);
	virtual void keyPressEvent(QKeyEvent *);
public slots:
	void onTextChanged(const QString & newText);
	void updateIcon(IconState iconState);
private:
	IconStorage * iconStorage;
	QIcon currentIcon;
	QLabel * iconLabel;
};

#endif // SEARCHEDIT_H
