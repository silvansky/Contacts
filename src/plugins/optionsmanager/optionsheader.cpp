#include "optionsheader.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QTextDocument>

OptionsHeader::OptionsHeader(const QString &AIconKey, const QString &ACaption, QWidget *AParent) : QFrame(AParent)
{
	setObjectName("wdtOptionsHeader");

	QHBoxLayout *hlayout = new QHBoxLayout(this);
	hlayout->setContentsMargins(0,hlayout->spacing()*1.5,0,0);

	QLabel *icon = new QLabel(this);
	icon->setObjectName("optionsIconLabel");
	icon->setFixedSize(20,20);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(icon,AIconKey,0,0,"pixmap");
	hlayout->addWidget(icon);
	if (!icon->pixmap())
		icon->setVisible(false);

	QLabel *caption = new QLabel(this);
	caption->setObjectName("optionsCaptionLabel");
	caption->setText(ACaption);
	hlayout->addWidget(caption);

	hlayout->addStretch();
}

OptionsHeader::~OptionsHeader()
{

}

void OptionsHeader::apply()
{
	emit childApply();
}

void OptionsHeader::reset()
{
	emit childReset();
}
