#ifndef STATISTICS_H
#define STATISTICS_H

#include "utilsexport.h"
#include <QObject>

class UTILS_EXPORT Statistics : public QObject
{
	Q_OBJECT
	class Counter;
private:
	Statistics();
public:
	static Statistics * instance();
	void initCounters();
	void release();
	void addCounter(const QString &url, bool image, int interval, QObject * delegate, const char * method);
protected slots:
	void onTimer();
private:
	static Statistics * inst;
	QList<Counter *> counters;
};

#endif // STATISTICS_H
