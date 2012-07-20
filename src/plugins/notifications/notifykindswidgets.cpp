#include "notifykindswidgets.h"

#include <QVBoxLayout>

NotifyKindsWidgets::NotifyKindsWidgets(QWidget *AParent) : QWidget(AParent)
{
	QVBoxLayout *vbl = new QVBoxLayout(this);
	setLayout(vbl);
	vbl->setSpacing(0);
	vbl->setContentsMargins(0, 0, 0, 0);
	setObjectName("notifyKindsWidgets");
}

void NotifyKindsWidgets::addWidget(IOptionsWidget *AWidget)
{
	connect(AWidget->instance(), SIGNAL(modified()), SIGNAL(modified()));
	connect(AWidget->instance(), SIGNAL(childApply()), SIGNAL(childApply()));
	connect(AWidget->instance(), SIGNAL(childReset()), SIGNAL(childReset()));
	layout()->addWidget(AWidget->instance());
}

void NotifyKindsWidgets::apply()
{
	foreach (QObject* child, children())
	{
		if (IOptionsWidget *widget = qobject_cast<IOptionsWidget*>(child))
		{
			widget->apply();
		}
	}
}

void NotifyKindsWidgets::reset()
{
	foreach (QObject* child, children())
	{
		if (IOptionsWidget *widget = qobject_cast<IOptionsWidget*>(child))
		{
			widget->reset();
		}
	}
}
