#ifndef ABOUTQTDIALOG_H
#define ABOUTQTDIALOG_H

#include <QWidget>

class AboutQtDialog : 
	public QWidget
{
	Q_OBJECT;
private:
	AboutQtDialog();
protected:
	void paintEvent(QPaintEvent *);
	void keyPressEvent(QKeyEvent *);
public:
	static void aboutQt();
};

#endif // ABOUTQTDIALOG_H
