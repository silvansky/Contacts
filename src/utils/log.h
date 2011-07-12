#ifndef LOG_H
#define LOG_H

#include "utilsexport.h"

#include <QString>

class QMutex;

class UTILS_EXPORT Log
{
public:
	enum LogFormat
	{
		None,
		Simple,
		HTML,
		Both
	};
	enum LogLevel
	{
		NoLog = 0,
		Errors = 1,
		Warnings = 2,
		Everything = 3
	};
	static void writeLog(const QString &, int _level);
	static QString logPath();
	static void setLogPath(const QString & newPath);
	static QString currentFileName();
	static LogFormat logFormat();
	static void setLogFormat(LogFormat newFormat);
	static LogLevel level();
	static void setLevel(LogLevel newLevel);
	static int maxLogSize();
	static void setMaxLogSize(int kbytes);
private:
	static QString path;
	static LogFormat format;
	static LogLevel currentLogLevel;
	static QString currentLogFile;
	static uint currentMaxLogSize;
	static QMutex mutex;
};

void UTILS_EXPORT Log(const QString &, int level = 3 /* common */);
void UTILS_EXPORT LogError(const QString &s);
void UTILS_EXPORT LogWarning(const QString &s);

#endif // LOG_H
