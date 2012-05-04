#include "statistics.h"

#include "networking.h"

#include <QTimer>
#include <QDateTime>
#include <definitions/optionvalues.h>

// private class Counter

class Statistics::Counter : public QObject
{
	Q_OBJECT
public:
	Counter(const QString &_url, bool _image, int _interval, bool execNow = false)
	{
		url = _url;
		image = _image;
		interval = _interval;
		id = getId();
		if (interval > 0)
		{
			connect(&timer, SIGNAL(timeout()), SLOT(onTimer()));
			timer.start(interval);
		}
		if (execNow || (interval <= 0))
			onTimer();
	}
	void stop()
	{
		timer.stop();
		deleteLater();
	}
	bool enabled() const
	{
		return timer.isActive();
	}
	~Counter()
	{
		stop();
	}
protected slots:
	void onTimer()
	{
		lastTimeout = QDateTime::currentDateTime();
		if (image)
			Networking::httpGetImageAsync(url, NULL, NULL);
		else
			Networking::httpGetStringAsync(url, NULL, NULL);
		if (!enabled())
			deleteLater();
	}
private:
	int getId()
	{
		static int i = 0;
		return ++i;
	}
public:
	int id;
	QString url;
	bool image;
	int interval;
	QTimer timer;
	QDateTime lastTimeout;
};

// class Statistics

Statistics * Statistics::inst = NULL;

Statistics::Statistics() :
	QObject(NULL)
{
	connect(Options::instance(), SIGNAL(optionsOpened()), SLOT(onOptionsOpened()));
	connect(Options::instance(), SIGNAL(optionsClosed()), SLOT(onOptionsClosed()));
}

Statistics::~Statistics()
{
	Counter * c;
	while (counters.count() && (c = counters.takeLast()))
	{
		c->stop();
	}
}

Statistics *Statistics::instance()
{
	if (!inst)
		inst = new Statistics;
	return inst;
}

void Statistics::initCounters()
{
	// TODO: init predefined counters
}

void Statistics::release()
{
	if (inst)
	{
		delete inst;
		inst = NULL;
	}
}

void Statistics::addCounter(const QString &url, bool image, int interval)
{
	Counter *c = new Counter(url, image, interval);
	connect(c, SIGNAL(destroyed()), SLOT(onCounterDestroyed()));
	counters << c;
}

void Statistics::onOptionsOpened()
{
}

void Statistics::onOptionsClosed()
{
}

void Statistics::onCounterDestroyed()
{
	Counter *c = qobject_cast<Counter *>(sender());
	if (c)
		counters.removeOne(c);
}

#include "statistics.moc"
