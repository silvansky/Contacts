#include "statistics.h"

// class Counter

class Counter : public QObject
{

};

// class Statistics

Statistics * Statistics::inst = NULL;

Statistics::Statistics() :
	QObject(NULL)
{
}

Statistics *Statistics::instance()
{
	if (!inst)
		inst = new Statistics;
	return inst;
}

void Statistics::initCounters()
{
}

void Statistics::release()
{
	if (inst)
	{
		delete inst;
		inst = NULL;
	}
}

void Statistics::addCounter(const QString &url, bool image, int interval, QObject *delegate, const char *method)
{
	Q_UNUSED(url)
	Q_UNUSED(image)
	Q_UNUSED(interval)
	Q_UNUSED(delegate)
	Q_UNUSED(method)
}

void Statistics::onTimer()
{
}
