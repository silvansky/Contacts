#include "customwebpage.h"

#include <QDebug>

CustomWebPage::CustomWebPage(QObject *parent) :
	QWebPage(parent)
{
	// setting default User-Agent
	setCustomUserAgent(QWebPage::userAgentForUrl(QUrl("http://rambler.ru")));
}

CustomWebPage::~CustomWebPage()
{
}

QString CustomWebPage::customUserAgent() const
{
	return _customUserAgent;
}

void CustomWebPage::setCustomUserAgent(const QString &newUserAgent)
{
	qDebug() << QString("Setting User-Agent to %1").arg(newUserAgent);
	_customUserAgent = newUserAgent;
}

QString CustomWebPage::customUserAgentForUrl(const QUrl &url) const
{
	return _customUserAgents.value(url, _customUserAgent);
}

void CustomWebPage::setCustomUserAgentForUrl(const QString &newUserAgent, const QUrl &url)
{
	_customUserAgents.insert(url, newUserAgent);
}

QString CustomWebPage::userAgentForUrl(const QUrl &url) const
{
	return _customUserAgents.value(url, _customUserAgent);
}
