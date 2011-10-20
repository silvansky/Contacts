#include "dialwidget.h"

#include <QClipboard>
#include <QApplication>
#include <QRegExpValidator>

DialWidget::DialWidget(ISipPhone *ASipPhone, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FSipPhone = ASipPhone;
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_DIALWIDGET);

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

	QRegExp phoneNumber("[\\d|\\*|\\#]+");
	ui.lneNumber->setValidator(new QRegExpValidator(phoneNumber,ui.lneNumber));

	ui.lneNumber->setAlignment(Qt::AlignHCenter);
	connect(ui.lneNumber,SIGNAL(textChanged(const QString &)),SLOT(onNumberTextChanged(const QString &)));
	onNumberTextChanged(ui.lneNumber->text());
}

DialWidget::~DialWidget()
{

}

void DialWidget::onButtonMapped(const QString &AText)
{
	ui.lneNumber->insert(AText);
	ui.lneNumber->setFocus();
}

void DialWidget::onNumberTextChanged(const QString &AText)
{

}
