#include "log.h"

#include <QFile>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QTextDocument>

// class Log

QString Log::path = QDir::homePath();
Log::LogFormat Log::format = Log::Both;

void Log::writeLog(const QString & s)
{
	QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

	// simple log
	if (format == Simple || format == Both)
	{
		QFile logFile(path + "/log.txt");
		logFile.open(QFile::WriteOnly | QFile::Append);
		logFile.write(QString("[%1]: %2\r\n").arg(timestamp, s).toUtf8());
		logFile.close();
	}

	// html log
	if (format == HTML || format == Both)
	{
		QFile logFile_html(path + "/log.html");
		bool html_existed = QFile::exists(logFile_html.fileName());
		logFile_html.open(QFile::WriteOnly | QFile::Append);
		// writing header if needed
		if (!html_existed)
		{
			logFile_html.write(QString("<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><title>VirtusMobile log</title></head>\r\n<body>\r\n").toUtf8());
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
	writeLog(QString("Log started at %1").arg(path));
}

Log::LogFormat Log::logFormat()
{
	return format;
}

void Log::setLogFormat(Log::LogFormat newFormat)
{
	format = newFormat;
}

// non-class

void Log(const QString &s)
{
	Log::writeLog(s);
}
