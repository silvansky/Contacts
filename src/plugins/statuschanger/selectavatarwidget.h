#ifndef SELECTAVATARWIDGET_H
#define SELECTAVATARWIDGET_H

#include <QWidget>

namespace Ui {
	class SelectAvatarWidget;
}

class SelectAvatarWidget : public QWidget {
	Q_OBJECT
public:
	SelectAvatarWidget(QWidget *parent = 0);
	~SelectAvatarWidget();

protected:
	void changeEvent(QEvent *e);
	bool eventFilter(QObject *, QEvent *);
signals:
	void avatarSelected(const QImage&);

private:
	Ui::SelectAvatarWidget *ui;

private slots:
    void on_profileButton_clicked();
    void on_uploadButton_clicked();
};

#endif // SELECTAVATARWIDGET_H
