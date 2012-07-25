#ifndef WELCOMESCREENWIDGET_H
#define WELCOMESCREENWIDGET_H

#include <QWidget>
#include <QGridLayout>

namespace Ui {
class WelcomeScreenWidget;
}

class WelcomeScreenWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit WelcomeScreenWidget(QWidget *parent = 0);
	~WelcomeScreenWidget();
protected:
	void paintEvent(QPaintEvent * pe);
	bool eventFilter(QObject * obj, QEvent * evt);
protected slots:
	void onAddPressed();
	void onTextChanged(const QString & text);
signals:
	void addressEntered(const QString & address);
	
//private:
public:
	Ui::WelcomeScreenWidget *ui;
	QGridLayout* registerButtonsLayout;
};

#endif // WELCOMESCREENWIDGET_H
