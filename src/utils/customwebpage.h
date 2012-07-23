#ifndef CUSTOMWEBPAGE_H
#define CUSTOMWEBPAGE_H

#include <QWebPage>
#include <QMap>

#include "utilsexport.h"

class UTILS_EXPORT CustomWebPage : public QWebPage
{
	Q_OBJECT
	Q_PROPERTY(QString customUserAgent READ customUserAgent WRITE setCustomUserAgent)
public:
	explicit CustomWebPage(QObject *parent = NULL);
	virtual ~CustomWebPage();
	// props
	QString customUserAgent() const;
	void setCustomUserAgent(const QString &newUserAgent);
	QString customUserAgentForUrl(const QUrl &url) const;
	void setCustomUserAgentForUrl(const QString &newUserAgent, const QUrl &url);
protected:
	virtual QString userAgentForUrl(const QUrl &url) const;
protected:
	QString _customUserAgent;
	QMap<QUrl, QString> _customUserAgents;
};

#endif // CUSTOMWEBPAGE_H
