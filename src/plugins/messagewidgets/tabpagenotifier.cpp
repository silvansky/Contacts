#include "tabpagenotifier.h"

TabPageNotifier::TabPageNotifier(ITabPage *ATabPage) : QObject(ATabPage->instance())
{
	FTabPage = ATabPage;

	FUpdateTimer.setInterval(0);
	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));
}

TabPageNotifier::~TabPageNotifier()
{
	while (!FNotifies.isEmpty())
		removeNotify(FNotifies.keys().first());
}

ITabPage *TabPageNotifier::tabPage() const
{
	return FTabPage;
}

int TabPageNotifier::activeNotify() const
{
	return FActiveNotify;
}

QList<int> TabPageNotifier::notifies() const
{
	return FNotifies.keys();
}

ITabPageNotify TabPageNotifier::notifyById(int ANotifyId) const
{
	return FNotifies.value(ANotifyId);
}

int TabPageNotifier::insertNotify(const ITabPageNotify &ANotify)
{
	if (ANotify.priority > 0)
	{
		int notifyId = qrand();
		while (notifyId<=0 || FNotifies.contains(notifyId))
			notifyId = qrand();
		FNotifies.insert(notifyId,ANotify);
		FUpdateTimer.start();
		emit notifyInserted(notifyId);
		return notifyId;
	}
	return -1;
}

void TabPageNotifier::removeNotify(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
		FUpdateTimer.start();
		emit notifyRemoved(ANotifyId);
	}
}

void TabPageNotifier::onUpdateTimerTimeout()
{
	int priority = -1;
	int notifyId = -1;
	for (QMap<int, ITabPageNotify>::const_iterator it = FNotifies.constBegin(); it!=FNotifies.constEnd(); it++)
	{
		if (it.value().priority > priority)
		{
			notifyId = it.key();
			priority = it.value().priority;
		}
	}

	if (notifyId != FActiveNotify)
	{
		FActiveNotify = notifyId;
		emit activeNotifyChanged(notifyId);
	}
}
