#include "log.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextDocument>
#include <QApplication>

// class Log
QMutex Log::FMutex;
uint Log::FLogTypes = 0;
uint Log::FMaxLogSize = 1024; // 1 MB by default
QString Log::FLogFile = QString::null;
QString Log::FLogPath = QDir::homePath();
Log::LogFormat Log::FLogFormat = Log::Simple;

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
		  abort();
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
			// ”станавливаем перехватчик Qt-шных сообщений только дл€ релиза, чтобы видеть во врем€ отладки
			qInstallMsgHandler(qtMessagesHandler);
#endif
			// creating name with current date: log_YYYY-MM-DD
			FLogFile = QString("log_%1").arg(QDate::currentDate().toString(Qt::ISODate));
			lock.unlock();
			writeMessage(Detaile,QString("-= Log started %1/%2.txt =-").arg(FLogPath,FLogFile));
			lock.relock();
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
