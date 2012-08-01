#include "statistics.h"

#include "networking.h"
#include "log.h"

#include <definitions/optionvalues.h>

#include <QTimer>
#include <QDateTime>

#ifdef DEBUG_STATISTICS
# include <QDebug>
#endif

#define COUNTER_KEY_ID			"id"
#define COUNTER_KEY_URL			"url"
#define COUNTER_KEY_IMAGE		"image"
#define COUNTER_KEY_INTERVAL		"interval"
#define COUNTER_KEY_LAST_TIMEOUT	"lastTimeout"

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
#ifdef DEBUG_STATISTICS
		qDebug() << QString("[Statistics::Counter]: Creating counter with url %1, interval %2 (assigned id %3)").arg(_url).arg(_interval).arg(id);
#endif
		LogDetail(QString("[Statistics::Counter]: Creating counter with url %1, interval %2 (assigned id %3)").arg(_url).arg(_interval).arg(id));
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
#ifdef DEBUG_STATISTICS
		qDebug() << QString("[Statistics::Counter]: Deleting counter %1 with url %2").arg(id).arg(url);
#endif
		LogDetail(QString("[Statistics::Counter]: Deleting counter %1 with url %2").arg(id).arg(url));
		timer.stop();
		if (!Statistics::isReleased())
			Statistics::instance()->removeCounter(this);
	}
protected slots:
	void onTimer()
	{
#ifdef DEBUG_STATISTICS
		qDebug() << QString("[Statistics::Counter]: Activating counter %1 with url %2").arg(id).arg(url);
#endif
		LogDetail(QString("[Statistics::Counter]: Activating counter %1 with url %2").arg(id).arg(url));
		lastTimeout = QDateTime::currentDateTime();
		if (image)
			Networking::httpGetImageAsync(url, NULL, NW_SLOT_NONE, NW_SLOT_NONE);
		else
			Networking::httpGetStringAsync(url, NULL, NW_SLOT_NONE, NW_SLOT_NONE);
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
bool Statistics::released = false;

Statistics::Statistics() :
	QObject(NULL)
{
	loadSettings();
}

Statistics::~Statistics()
{
	saveSettings();
	clearCounters();
}

Statistics *Statistics::instance()
{
	if (!inst)
	{
		inst = new Statistics;
		released = false;
	}
	return inst;
}

void Statistics::initCounters()
{
	// TODO: init predefined counters
}

void Statistics::release()
{
#ifdef DEBUG_STATISTICS
	qDebug() << "[Statistics::release]: Releasing statistics";
#endif
	if (inst)
	{
		delete inst;
		inst = NULL;
	}
	released = true;
}

void Statistics::addCounter(const QString &url, CounterContentType type, int interval)
{
	Counter *c = new Counter(url, (type == Image), interval, true);
	counters << c;
}

bool Statistics::isReleased()
{
	return released;
}

void Statistics::removeCounter(Counter *c)
{
	counters.removeAll(c);
}

void Statistics::clearCounters()
{
	Counter *c;
	while (counters.count() && (c = counters.takeLast()))
	{
		c->stop();
	}
}

void Statistics::saveSettings() const
{
	QVariantList settings;
	foreach (Counter *c, counters)
	{
#ifdef DEBUG_STATISTICS
		qDebug() << QString("[Statistics::saveSettings]: Saving state of counter %1 (url %2)").arg(c->id).arg(c->url);
#endif
		QVariantMap counterSettings;
		counterSettings.insert(COUNTER_KEY_ID, c->id);
		counterSettings.insert(COUNTER_KEY_URL, c->url);
		counterSettings.insert(COUNTER_KEY_IMAGE, c->image);
		counterSettings.insert(COUNTER_KEY_INTERVAL, c->interval);
		if (c->lastTimeout.isValid())
			counterSettings.insert(COUNTER_KEY_LAST_TIMEOUT, c->lastTimeout);
		settings << counterSettings;
	}
	//Options::instance()->setFileValue(settings, OPV_STATISTICS_COUNTERS);
	Options::setGlobalValue("statistics/counters", settings);
}

void Statistics::loadSettings()
{
	QVariant var = Options::globalValue("statistics/counters");
	if (var.isValid())
	{
		QVariantList settings = var.value<QVariantList>();
		clearCounters();
		foreach (QVariant v, settings)
		{
			QVariantMap counterSettings = v.value<QVariantMap>();
			int id = counterSettings.value(COUNTER_KEY_ID).toInt();
			QString url = counterSettings.value(COUNTER_KEY_URL).toString();
			bool image = counterSettings.value(COUNTER_KEY_IMAGE).toBool();
			int interval = counterSettings.value(COUNTER_KEY_INTERVAL).toInt();
			QDateTime lastTimeout = counterSettings.value(COUNTER_KEY_LAST_TIMEOUT).toDateTime();
			bool execNow = lastTimeout.msecsTo(QDateTime::currentDateTime()) > interval;
#ifdef DEBUG_STATISTICS
			qDebug() << QString("[Statistics::loadSettings]: Loading state of counter %1 (url %2)").arg(id).arg(url);
#endif
			Counter *c = new Counter(url, image, interval, execNow);
			c->id = id;
			c->lastTimeout = lastTimeout;
			counters << c;
		}
	}
}

#include "statistics.moc"
