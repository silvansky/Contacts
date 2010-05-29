#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <QLineEdit>
#include <utils/iconstorage.h>
#include <QLabel>

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

protected:
	void resizeEvent(QResizeEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
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
