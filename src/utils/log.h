#ifndef LOG_H
#define LOG_H

#include "utilsexport.h"

#include <QString>

class UTILS_EXPORT Log
{
public:
	static void writeLog(const QString &);
	static QString logPath();
	static void setLogPath(const QString & newPath);
private:
	static QString path;
};

void UTILS_EXPORT Log(const QString &);

#endif // LOG_H
