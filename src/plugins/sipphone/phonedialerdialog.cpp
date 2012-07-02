#include "phonedialerdialog.h"

#include <QClipboard>
#include <QApplication>
#include <QRegExpValidator>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

PhoneDialerDialog::PhoneDialerDialog(ISipManager *ASipManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	FSipManager = ASipManager;
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_PHONEDIALERDIALOG);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setMovable(true);
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
	}
	else
	{
		ui.lblCaption->setVisible(false);
		setAttribute(Qt::WA_DeleteOnClose,true);
	}

	window()->setWindowTitle(tr("Calls"));

	FMapper.setMapping(ui.pbtNumber_1,"1");
	connect(ui.pbtNumber_1,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_2,"2");
	connect(ui.pbtNumber_2,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_3,"3");
	connect(ui.pbtNumber_3,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_4,"4");
	connect(ui.pbtNumber_4,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_5,"5");
	connect(ui.pbtNumber_5,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_6,"6");
	connect(ui.pbtNumber_6,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_7,"7");
	connect(ui.pbtNumber_7,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_8,"8");
	connect(ui.pbtNumber_8,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_9,"9");
	connect(ui.pbtNumber_9,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_10,"*");
	connect(ui.pbtNumber_10,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_11,"0");
	connect(ui.pbtNumber_11,SIGNAL(clicked()),&FMapper,SLOT(map()));
	FMapper.setMapping(ui.pbtNumber_12,"#");
	connect(ui.pbtNumber_12,SIGNAL(clicked()),&FMapper,SLOT(map()));
	connect(&FMapper,SIGNAL(mapped(const QString &)),SLOT(onButtonMapped(const QString &)));

	QRegExp phoneNumber("\\+?[\\d|\\*|\\#]+");
	ui.lneNumber->setValidator(new QRegExpValidator(phoneNumber,ui.lneNumber));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtCall,MNI_SIPPHONE_DIALER_CALL);

	connect(ui.lneNumber,SIGNAL(textChanged(const QString &)),SLOT(onNumberTextChanged(const QString &)));
	onNumberTextChanged(ui.lneNumber->text());
}

PhoneDialerDialog::~PhoneDialerDialog()
{

}

void PhoneDialerDialog::onButtonMapped(const QString &AText)
{
	ui.lneNumber->insert(AText);
	ui.lneNumber->setFocus();
}

void PhoneDialerDialog::onNumberTextChanged(const QString &AText)
{

}
