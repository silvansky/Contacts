#ifndef TESTSTYLESFORM_H
#define TESTSTYLESFORM_H

#include <QWidget>

namespace Ui {
class TestStylesForm;
}

class TestStylesForm : public QWidget
{
	Q_OBJECT

public:
	explicit TestStylesForm(QWidget *parent = 0);
	~TestStylesForm();

private:
	Ui::TestStylesForm *ui;
};

#endif // TESTSTYLESFORM_H
