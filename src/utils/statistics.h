#ifndef STATISTICS_H
#define STATISTICS_H

#include "utilsexport.h"
#include "options.h"
#include <QObject>

class UTILS_EXPORT Statistics : public QObject
{
	Q_OBJECT
	class Counter;
private:
	Statistics();
public:
	virtual ~Statistics();
	static Statistics * instance();
	void initCounters();
	void release();
	void addCounter(const QString &url, bool image, int interval);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onCounterDestroyed();
private:
	static Statistics * inst;
	QList<Counter *> counters;
};

#endif // STATISTICS_H
