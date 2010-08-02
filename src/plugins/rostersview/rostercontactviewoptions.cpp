#include "rostercontactviewoptions.h"

RosterContactViewOptions::RosterContactViewOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	connect(ui.rdbViewFull,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbViewSimplified,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbViewCompact,SIGNAL(toggled(bool)),SIGNAL(modified()));

	// TODO: render index in RosterIndexDelegate for translatable results
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewFull,MNI_ROSTERVIEW_VIEW_FULL,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewSimplified,MNI_ROSTERVIEW_VIEW_SIMPLIFIED,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewCompact,MNI_ROSTERVIEW_VIEW_COMPACT,0,0,"pixmap");

	reset();
}

RosterContactViewOptions::~RosterContactViewOptions()
{

}

void RosterContactViewOptions::apply()
{
	if (ui.rdbViewFull->isChecked())
	{
		Options::node(OPV_AVATARS_SHOW).setValue(true);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(true);
	}
	else if (ui.rdbViewSimplified->isChecked())
	{
		Options::node(OPV_AVATARS_SHOW).setValue(true);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
	}
	else
	{
		Options::node(OPV_AVATARS_SHOW).setValue(false);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
	}
	emit childApply();
}

void RosterContactViewOptions::reset()
{
	if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool() && Options::node(OPV_AVATARS_SHOW).value().toBool())
		ui.rdbViewFull->setChecked(true);
	else if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool())
		ui.rdbViewSimplified->setChecked(true);
	else
		ui.rdbViewCompact->setChecked(true);

	emit childReset();
}
