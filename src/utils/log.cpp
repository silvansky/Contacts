#include "log.h"

#include <QFile>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QTextDocument>
#include <QMutex>

// class Log

QString Log::path = QDir::homePath();
Log::LogFormat Log::format = Log::Both;
Log::LogLevel Log::currentLogLevel = Log::Errors;
QString Log::currentLogFile = QString::null;
uint Log::currentMaxLogSize = 1024; // 1 MB by default
QMutex Log::mutex;

void Log::writeLog(const QString & s, int _level)
{
	QMutexLocker lock(&mutex);
	Q_UNUSED(lock);

	// checking level
	if (!currentLogLevel || (_level > currentLogLevel))
		return;

	// checking format
	if (format == None)
		return;

	if (currentLogFile.isNull())
	{
		// creating name with current date: log_YYYY-MM-DD
		currentLogFile = QString("log_%1").arg(QDate::currentDate().toString(Qt::ISODate));
		writeLog(QString("Log started at %1").arg(path), Errors);
	}

	QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

	// simple log
	if (format == Simple || format == Both)
	{
		QFile logFile(path + "/" + currentLogFile + ".txt");
		logFile.open(QFile::WriteOnly | QFile::Append);
		logFile.write(QString("[%1]: %2\r\n").arg(timestamp, s).toUtf8());
		logFile.close();
	}

	// html log
	if (format == HTML || format == Both)
	{
		QFile logFile_html(path + "/" + currentLogFile + ".html");
		bool html_existed = QFile::exists(logFile_html.fileName());
		logFile_html.open(QFile::WriteOnly | QFile::Append);
		// writing header if needed
		if (!html_existed)
		{
			logFile_html.write(QString("<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><title>Rambler Contacts log</title></head>\r\n<body>\r\n").toUtf8());
		}
		// if we got '[' at the beginning, we gonna highlight it in html output (till ']')
		QString htmlLog;
		if (s[0] == '[')
		{
			int i = s.indexOf(']');
			if (i != -1)
			{
				QString highlighted = s.left(i + 1);
				htmlLog = QString("<p><pre><b>[%1]</b>: %2%3</pre></p>\r\n").arg(timestamp, QString("<font color=red>%1</font>").arg(highlighted), Qt::escape(s.right(s.length() - i - 1)));
			}
		}
		else
			htmlLog = QString("<p><pre><b>[%1]</b>: %2</pre></p>\r\n").arg(timestamp, Qt::escape(s));
		logFile_html.write(htmlLog.toUtf8());
		logFile_html.close();
	}
}

QString Log::logPath()
{
	return path;
}

void Log::setLogPath(const QString & newPath)
{
	path = newPath;
}

QString Log::currentFileName()
{
	return currentLogFile;
}

Log::LogFormat Log::logFormat()
{
	return format;
}

void Log::setLogFormat(Log::LogFormat newFormat)
{
	format = newFormat;
}

Log::LogLevel Log::level()
{
	return currentLogLevel;
}

void Log::setLevel(LogLevel newLevel)
{
	currentLogLevel = newLevel;
}

int Log::maxLogSize()
{
	return currentMaxLogSize;
}

void Log::setMaxLogSize(int kbytes)
{
	currentMaxLogSize = kbytes;
}

// non-class

void Log(const QString &s, int level)
{
	Log::writeLog(s, level);
}

void LogError(const QString &s)
{
	Log(s, Log::Errors);
}

void LogWarning(const QString &s)
{
	Log(s, Log::Warnings);
}
