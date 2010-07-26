#include "legacyaccountoptions.h"

LegacyAccountOptions::LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FGateways = AGateways;
	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;
}

LegacyAccountOptions::~LegacyAccountOptions()
{

}
