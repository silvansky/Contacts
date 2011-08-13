#ifndef LOG_H
#define LOG_H

#include "utilsexport.h"

#include <QMutex>
#include <QString>

class UTILS_EXPORT Log
{
public:
	enum LogFormat {
		Simple,
		HTML,
		Both
	};
	enum LogType {
		Error         = 0x0001,
		Warning       = 0x0002,
		Detaile       = 0x0004,
		Stanza        = 0x0008,
		Debug         = 0x0010
	};
	static QString logFileName();
	static QString logPath();
	static void setLogPath(const QString &APath);
	static LogFormat logFormat();
	static void setLogFormat(LogFormat AFormap);
	static uint logTypes();
	static void setLogTypes(uint AType);
	static int maxLogSize();
	static void setMaxLogSize(int AKBytes);
	static void writeMessage(uint AType, const QString &AMessage);
private:
	static QString FLogFile;
	static QString FLogPath;
	static uint FLogTypes;
	static LogFormat FLogFormat;
	static uint FMaxLogSize;
	static QMutex FMutex;
};

void UTILS_EXPORT LogError(const QString &AMessage);
void UTILS_EXPORT LogWarning(const QString &AMessage);
void UTILS_EXPORT LogDetaile(const QString &AMessage);
void UTILS_EXPORT LogStanza(const QString &AMessage);
void UTILS_EXPORT LogDebug(const QString &AMessage);

#endif // LOG_H
