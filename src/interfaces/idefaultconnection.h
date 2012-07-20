#ifndef IDEFAULTCONNECTION_H
#define IDEFAULTCONNECTION_H

#define DEFAULTCONNECTION_UUID "{837bab61-dc61-43d7-932f-a573af12d0b4}"

#include <QSslSocket>

class IDefaultConnection
{
public:
	enum OptionRoles {
		COR_HOST,
		COR_PORT,
		COR_DOMAINE,
		COR_USE_SSL,
		COR_IGNORE_SSL_ERRORS,
		COR_CHANGE_PROXY_TYPE
	};
public:
	virtual QObject *instance() =0;
	virtual void startClientEncryption() =0;
	virtual QSsl::SslProtocol protocol() const =0;
	virtual void setProtocol(QSsl::SslProtocol AProtocol) =0;
	virtual void addCaCertificate(const QSslCertificate &ACertificate) =0;
	virtual QList<QSslCertificate> caCertificates() const =0;
	virtual QSslCertificate peerCertificate() const =0;
	virtual void ignoreSslErrors() =0;
	virtual QList<QSslError> sslErrors() const =0;
	virtual QNetworkProxy proxy() const =0;
	virtual void setProxy(const QNetworkProxy &AProxy) =0;
	virtual QVariant option(int ARole) const =0;
	virtual void setOption(int ARole, const QVariant &AValue) =0;
	virtual QString localAddress() =0;
protected:
	virtual void modeChanged(QSslSocket::SslMode AMode) =0;
	virtual void sslErrors(const QList<QSslError> &AErrors) =0;
	virtual void proxyChanged(const QNetworkProxy &AProxy) =0;
};

class IDefaultConnectionPlugin
{
public:
	virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IDefaultConnection,"Virtus.Plugin.IDefaultConnection/1.0")
Q_DECLARE_INTERFACE(IDefaultConnectionPlugin,"Virtus.Plugin.IDefaultConnectionPlugin/1.0")

#endif
