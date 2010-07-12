#include "addlegacyaccountoptions.h"

AddLegacyAccountOptions::AddLegacyAccountOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
}

AddLegacyAccountOptions::~AddLegacyAccountOptions()
{

}

void AddLegacyAccountOptions::apply()
{
	emit childApply();
}

void AddLegacyAccountOptions::reset()
{
	emit childReset();
}
