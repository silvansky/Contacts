#ifndef ADDACCOUNTWIDGET_H
#define ADDACCOUNTWIDGET_H

#include <QWidget>

namespace Ui {
class AddAccountWidget;
}

class AddAccountWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit AddAccountWidget(QWidget *parent = 0);
	~AddAccountWidget();
	
private:
	Ui::AddAccountWidget *ui;
};

#endif // ADDACCOUNTWIDGET_H
