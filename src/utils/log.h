#ifndef LOG_H
#define LOG_H

#include "utilsexport.h"

#include <QString>

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
	static void writeLog(const QString &);
	static QString logPath();
	static void setLogPath(const QString & newPath);
	static LogFormat logFormat();
	static void setLogFormat(LogFormat newFormat);
private:
	static QString path;
	static LogFormat format;
};

void UTILS_EXPORT Log(const QString &);

#endif // LOG_H
