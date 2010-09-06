#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <utils/iconstorage.h>

class SearchEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit SearchEdit(QWidget *parent = 0);
	enum IconState
	{
		Ready,
		InProgress,
		Hover
	};
	void processKeyPressEvent(QKeyEvent * event);

protected:
	void resizeEvent(QResizeEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
	void leaveEvent(QEvent *);
private:
	IconStorage * iconStorage;
	QIcon currentIcon;
	QLabel * iconLabel;
signals:

public slots:
	void onTextChanged(const QString & newText);
	void updateIcon(IconState iconState);

};

#endif // SEARCHEDIT_H
