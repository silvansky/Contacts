#include "notifykindswidgets.h"

#include <QVBoxLayout>

NotifyKindsWidgets::NotifyKindsWidgets(QWidget * parent) :
	QWidget(parent)
{
	QVBoxLayout * vbl = new QVBoxLayout(this);
	setLayout(vbl);
	vbl->setSpacing(0);
	vbl->setContentsMargins(0, 0, 0, 0);
	setObjectName("notifyKindsWidgets");
}

void NotifyKindsWidgets::addWidget(IOptionsWidget * widget)
{
	connect(widget->instance(), SIGNAL(modified()), SIGNAL(modified()));
	connect(widget->instance(), SIGNAL(childApply()), SIGNAL(childApply()));
	connect(widget->instance(), SIGNAL(childReset()), SIGNAL(childReset()));
	layout()->addWidget(widget->instance());
}

void NotifyKindsWidgets::apply()
{
	foreach (QObject* child, children())
	{
		if (IOptionsWidget * ow = qobject_cast<IOptionsWidget*>(child))
		{
			ow->apply();
		}
	}
}

void NotifyKindsWidgets::reset()
{
	foreach (QObject* child, children())
	{
		if (IOptionsWidget * ow = qobject_cast<IOptionsWidget*>(child))
		{
			ow->reset();
		}
	}
}
