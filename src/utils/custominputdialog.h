#ifndef CUSTOMINPUTDIALOG_H
#define CUSTOMINPUTDIALOG_H

#include <QInputDialog>
#include "utilsexport.h"
#include "custombordercontainer.h"

class UTILS_EXPORT CustomInputDialog : public QInputDialog
{
	Q_OBJECT
public:
	explicit CustomInputDialog(QWidget *parent = 0);
	~CustomInputDialog();
	CustomBorderContainer * windowBorder();
	void show();
private:
	CustomBorderContainer * border;
};

#endif // CUSTOMINPUTDIALOG_H
