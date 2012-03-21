#ifndef LOG_H
#define LOG_H

#include "utilsexport.h"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QDomDocument>

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
public:
	static QString generateBacktrace();
public:
	static void setStaticReportParam(const QString &AKey, const QString &AValue);
	static QDomDocument generateReport(QMap<QString, QString> &AParams, bool AIncludeLog = true);
	static bool sendReport(QDomDocument AReport);
private:
	static QMutex FMutex;
	static uint FLogTypes;
	static uint FMaxLogSize;
	static QString FLogFile;
	static QString FLogPath;
	static LogFormat FLogFormat;
	static QMap<QString,QString> FReportParams;
};

void UTILS_EXPORT LogError(const QString &AMessage);
void UTILS_EXPORT LogWarning(const QString &AMessage);
void UTILS_EXPORT LogDetail(const QString &AMessage);
void UTILS_EXPORT LogStanza(const QString &AMessage);
void UTILS_EXPORT LogDebug(const QString &AMessage);

void UTILS_EXPORT ReportError(const QString &ACode, const QString &ADescr, bool AIncludeLog = true);

#endif // LOG_H
