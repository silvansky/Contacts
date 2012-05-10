#ifndef STATISTICS_H
#define STATISTICS_H

#include "utilsexport.h"
#include "options.h"
#include <QObject>

class UTILS_EXPORT Statistics : public QObject
{
	Q_OBJECT
	class Counter;
	friend class Counter;
private:
	Statistics();
public:
	enum CounterContentType
	{
		Text,
		Image
	};
	virtual ~Statistics();
	static Statistics * instance();
	static void release();
	void initCounters();
	void addCounter(const QString &url, CounterContentType type, int interval);
protected:
	static bool isReleased();
	void removeCounter(Counter *c);
	void clearCounters();
	void saveSettings() const;
	void loadSettings();
private:
	static Statistics * inst;
	static bool released;
	QList<Counter *> counters;
};

#endif // STATISTICS_H
