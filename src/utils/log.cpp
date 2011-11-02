#include "log.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include <QDateTime>
#include <QApplication>
#include <QTextDocument>
#include <definitions/version.h>
#include <definitions/applicationreportparams.h>
#include "datetime.h"
#include "systemmanager.h"

#define APP_REPORT_VERSION         "1.0"
#define DIR_HOLDEM_REPORTS         "Rambler/Holdem/Reports"

// class Log
QMutex Log::FMutex;
uint Log::FLogTypes = 0;
uint Log::FMaxLogSize = 1024; // 1 MB by default
QString Log::FLogFile = QString::null;
QString Log::FLogPath = QDir::homePath();
Log::LogFormat Log::FLogFormat = Log::Simple;
QMap<QString,QString> Log::FReportParams;

void qtMessagesHandler(QtMsgType AType, const char *AMessage)
{
	switch (AType) 
	{
	case QtDebugMsg:
		LogDebug(QString("[QtDebugMsg] %1").arg(AMessage));
		break;
	case QtWarningMsg:
		LogWarning(QString("[QtWarningMsg] %1").arg(AMessage));
		break;
	case QtCriticalMsg:
		LogError(QString("[QtCriticalMsg] %1").arg(AMessage));
		break;
	case QtFatalMsg:
		LogError(QString("[QtFatalMsg] %1").arg(AMessage));
		break;
	}
}

QString Log::logFileName()
{
	return FLogFile;
}

QString Log::logPath()
{
	return FLogPath;
}

void Log::setLogPath(const QString &APath)
{
	if (FLogPath != APath)
	{
		FLogPath = APath;
		FLogFile = QString::null;
	}
}

Log::LogFormat Log::logFormat()
{
	return FLogFormat;
}

void Log::setLogFormat(Log::LogFormat AFormat)
{
	FLogFormat = AFormat;
}

uint Log::logTypes()
{
	return FLogTypes;
}

void Log::setLogTypes(uint ALevel)
{
	FLogTypes = ALevel;
}

int Log::maxLogSize()
{
	return FMaxLogSize;
}

void Log::setMaxLogSize(int AKBytes)
{
	FMaxLogSize = AKBytes;
}

// TODO: truncate log when it's over currentMaxLogSize
void Log::writeMessage(uint AType, const QString &AMessage)
{
	if (!FLogPath.isEmpty() && (FLogTypes & AType)>0)
	{
		static QDateTime lastMessageTime = QDateTime::currentDateTime();
		QMutexLocker lock(&FMutex);

		if (FLogFile.isNull())
		{
#ifndef DEBUG_ENABLED
			// Устанавливаем перехватчик Qt-шных сообщений только для релиза, чтобы видеть во время отладки
			qInstallMsgHandler(qtMessagesHandler);
#endif
			// creating name with current date and time: log_YYYY-MM-DDTHH-MM-SS+TZ.txt
			FLogFile = QString("log_%1").arg(DateTime(QDateTime::currentDateTime()).toX85DateTime().replace(":","-"));
		}

		QDateTime curDateTime = QDateTime::currentDateTime();
		QString timestamp = curDateTime.toString("hh:mm:ss.zzz");
		int timedelta = lastMessageTime.msecsTo(curDateTime);
		lastMessageTime = curDateTime;

		// simple log
		if (FLogFormat == Simple || FLogFormat == Both)
		{
			QFile logFile(FLogPath + "/" + FLogFile + ".txt");
			logFile.open(QFile::WriteOnly | QFile::Append);
			logFile.write(QString("%1\t+%2\t[%3]\t%4\r\n").arg(timestamp).arg(timedelta).arg(AType).arg(AMessage).toUtf8());
			logFile.close();
		}

		// html log
		if (FLogFormat == HTML || FLogFormat == Both)
		{
			QFile logFile_html(FLogPath + "/" + FLogFile + ".html");
			bool html_existed = QFile::exists(logFile_html.fileName());
			logFile_html.open(QFile::WriteOnly | QFile::Append);
			// writing header if needed
			if (!html_existed)
			{
				logFile_html.write(QString("<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><title>Rambler Contacts log</title></head>\r\n<body>\r\n").toUtf8());
			}
			// if we got '[' at the beginning, we gonna highlight it in html output (till ']')
			QString htmlLog;
			QString marker;
			if (AType == Error)
				marker = "red";
			else if (AType == Warning)
				marker = "orange";
			else marker = "black";
			marker = QString("<font color=%1>&#9679;</font>").arg(marker);
			if (AMessage[0] == '[')
			{
				int i = AMessage.indexOf(']');
				if (i != -1)
				{
					QString highlighted = AMessage.left(i + 1);
					htmlLog = QString("<p><pre><b>[%1]</b>: %2 %3%4</pre></p>\r\n").arg(timestamp, marker, QString("<font color=orange>%1</font>").arg(highlighted), Qt::escape(AMessage.right(AMessage.length() - i - 1)));
				}
			}
			else
				htmlLog = QString("<p><pre><b>[%1]</b>: %2 %3</pre></p>\r\n").arg(timestamp, marker, Qt::escape(AMessage));
			logFile_html.write(htmlLog.toUtf8());
			logFile_html.close();
		}
	}
}

void Log::setStaticReportParam(const QString &AKey, const QString &AValue)
{
	if (!AValue.isNull())
		FReportParams.insert(AKey,AValue);
	else
		FReportParams.remove(AKey);
}

QDomDocument Log::generateReport(QMap<QString, QString> &AParams, bool AIncludeLog)
{
	QDomDocument report;

	// Заполняем общие параметры
	AParams.insert(ARP_REPORTTIME,DateTime(QDateTime::currentDateTime()).toX85DateTime());
	
	AParams.insert(ARP_APPLICATION_GUID,CLIENT_GUID);
	AParams.insert(ARP_APPLICATION_NAME,CLIENT_NAME);
	AParams.insert(ARP_APPLICATION_VERSION,CLIENT_VERSION);
	
	AParams.insert(ARP_SYSTEM_OSVERSION,SystemManager::systemOSVersion());
	AParams.insert(ARP_SYSTEM_QTVERSIONRUN,qVersion());
	AParams.insert(ARP_SYSTEM_QTVERSIONBUILD,QT_VERSION_STR);

	AParams.insert(ARP_LOCALE_NAME,QLocale().name());
	
	// Добавляем фаил лога
	if (AIncludeLog && !FLogFile.isEmpty())
	{
		QFile file(FLogPath + "/" + FLogFile + ".txt");
		if (file.open(QFile::ReadOnly))
		{
			QByteArray data = file.readAll();
			if (!data.isEmpty())
			{
				AParams.insert(ARP_FILE_LOG_NAME,FLogFile);
				AParams.insert(ARP_FILE_LOG_BASE64,data.toBase64());
			}
			file.close();
		}
	}

	// Добавляем заранее установленные параметры
	for (QMap<QString,QString>::const_iterator it = FReportParams.constBegin(); it!=FReportParams.constEnd(); it++)
	{
		if (!AParams.contains(it.key()))
			AParams.insert(it.key(),it.value());
	}

	// Создаем XML документ отчета
	QDomElement reportElem = report.appendChild(report.createElement("report")).toElement();
	reportElem.setAttribute("version",APP_REPORT_VERSION);
	for (QMap<QString,QString>::const_iterator it = AParams.constBegin(); it!=AParams.constEnd(); it++)
	{
		QDomElement paramElem = reportElem;
		QStringList paramPath = it.key().split(".",QString::SkipEmptyParts);
		foreach(QString paramItem, paramPath)
		{
			QDomElement itemElem = paramElem.firstChildElement(paramItem);
			if (itemElem.isNull())
				paramElem = paramElem.appendChild(report.createElement(paramItem)).toElement();
			else
				paramElem = itemElem;
		}
		paramElem.appendChild(report.createTextNode(it.value()));
	}

	return report;
}

bool Log::sendReport(QDomDocument AReport)
{
	if (!AReport.isNull())
	{
		QString dirPath = QDir::homePath();
		foreach(QString env, QProcess::systemEnvironment())
		{
			if (env.startsWith("APPDATA="))
				dirPath = env.split("=").value(1);
		}

		QDir dir(dirPath);
		if (dir.exists() && (dir.exists(DIR_HOLDEM_REPORTS) || dir.mkpath(DIR_HOLDEM_REPORTS)) && dir.cd(DIR_HOLDEM_REPORTS))
		{
			QString fileName = QString("contacts_%1.xml").arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate).replace(":","="));
			QFile file(dir.absoluteFilePath(fileName));
			if (file.open(QFile::WriteOnly|QFile::Truncate))
			{
				file.write(AReport.toString(3).toUtf8());
				file.close();
				return true;
			}
		}
	}
	return false;
}

// non-class
void LogError(const QString &AMessage)
{
	Log::writeMessage(Log::Error, AMessage);
}

void LogWarning(const QString &AMessage)
{
	Log::writeMessage(Log::Warning, AMessage);
}

void LogDetaile(const QString &AMessage)
{
	Log::writeMessage(Log::Detaile, AMessage);
}

void LogStanza(const QString &AMessage)
{
	QString stanzaText = AMessage;
	if (stanzaText.contains("mechanism=\"PLAIN\""))
	{
		// removing plain password
		int start = stanzaText.indexOf('>');
		int end = stanzaText.indexOf('<', start + 1);
		stanzaText.replace(start + 1, end - start, "PLAIN_LOGIN_AND_PASSWORD");
	}
	Log::writeMessage(Log::Stanza, stanzaText);
}

void LogDebug(const QString &AMessage)
{
	Log::writeMessage(Log::Debug, AMessage);
}
