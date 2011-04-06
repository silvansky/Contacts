#include "custominputdialog.h"

#include "customborderstorage.h"
#include <definitions/customborder.h>
#include <definitions/resources.h>

CustomInputDialog::CustomInputDialog(QWidget *parent) :
	QInputDialog(parent)
{
	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		layout()->setContentsMargins(14, 14, 14, 10);
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
}

CustomInputDialog::~CustomInputDialog()
{
	if (border)
		border->deleteLater();
}

CustomBorderContainer * CustomInputDialog::windowBorder()
{
	return border;
}

void CustomInputDialog::show()
{
	if (border)
	{
		// TODO: determine what of these are really needed
		border->layout()->update();
		layout()->update();
		border->adjustSize();
		border->show();
		border->layout()->update();
		border->adjustSize();
	}
	else
		QInputDialog::show();
}
