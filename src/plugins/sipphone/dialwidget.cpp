#include "dialwidget.h"

DialWidget::DialWidget(ISipPhone *ASipPhone, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FSipPhone = ASipPhone;
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_DIALWIDGET);
}

DialWidget::~DialWidget()
{

}
