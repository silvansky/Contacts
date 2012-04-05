#include "tunnelthread.h"

#include <QStringList>
#include <QXmlStreamReader>

enum ThreadState {
	TS_INIT,
	TS_PROXY_CONNECT,
	TS_PROXY_GETKEY,
	TS_REMOTE_CONNECT,
	TS_CONNECTED,
	TS_DISCONNECTING,
	TS_DISCONNECTED,
	TS_ABORTING,
	TS_ERROR
};

TunnelThread::TunnelThread(const ConnectRequest &ARequest, QObject *AParent)	: QThread(AParent)
{
	FRequest = ARequest;
	FThreadState = TS_INIT;
}

TunnelThread::~TunnelThread()
{

}

void TunnelThread::abort()
{
	quit();
}

void TunnelThread::run()
{
	FProxySocket = new QSslSocket();
	FProxySocket->setProtocol(QSsl::AnyProtocol);
	FProxySocket->setProxy(FRequest.connectProxy);
	FProxySocket->setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(FProxySocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(FProxySocket,SIGNAL(sslErrors(const QList<QSslError> &)),FProxySocket,SLOT(ignoreSslErrors()));

	FRemoteSocket = new QSslSocket();
	FRemoteSocket->setProtocol(QSsl::AnyProtocol);
	FRemoteSocket->setProxy(FRequest.connectProxy);
	FRemoteSocket->setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(FRemoteSocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(FRemoteSocket,SIGNAL(sslErrors(const QList<QSslError> &)),FRemoteSocket,SLOT(ignoreSslErrors()));

	FThreadState = TS_PROXY_CONNECT;
	if (!FRequest.proxyEncrypted)
		FProxySocket->connectToHost(FRequest.proxyHost,FRequest.proxyPort);
	else
		FProxySocket->connectToHostEncrypted(FRequest.proxyHost,FRequest.proxyPort);

	if (FProxySocket->waitForConnected() && (!FRequest.proxyEncrypted || FProxySocket->waitForEncrypted()))
	{
		FThreadState = TS_PROXY_GETKEY;

		QStringList elemStack;
		QXmlStreamReader reader;
		while(FThreadState==TS_PROXY_GETKEY && FProxySocket->waitForReadyRead())
		{
			reader.addData(FProxySocket->read(FProxySocket->bytesAvailable()));
			while (!reader.atEnd())
			{
				QString elemPath = elemStack.join("/");
				if (reader.isStartElement())
				{
					elemStack.append(reader.qualifiedName().toString());
				}
				else if (reader.isCharacters())
				{
					if (elemPath == "proxy/sessionkey")
					{
						FSessionKey = reader.text().toString();
					}
				}
				else if (reader.isEndElement())
				{
					elemStack.removeLast();
				}
				else if (reader.isEndDocument())
				{
					FThreadState = TS_REMOTE_CONNECT;
				}
			}

			if (reader.hasError() && reader.error()!=QXmlStreamReader::PrematureEndOfDocumentError)
				setErrorCondition("proxy-xml-not-well-formed");
		}

		if (FSessionKey.isEmpty())
			setErrorCondition("proxy-invalid-session-key");

		if (FThreadState == TS_REMOTE_CONNECT)
		{
			if (!FRequest.remoteEncrypted)
				FRemoteSocket->connectToHost(FRequest.remoteHost,FRequest.remotePort);
			else
				FRemoteSocket->connectToHostEncrypted(FRequest.remoteHost,FRequest.remotePort);

			if (FRemoteSocket->waitForConnected() && (!FRequest.remoteEncrypted || FRemoteSocket->waitForEncrypted()))
			{
				FThreadState = TS_CONNECTED;
				connect(FProxySocket,SIGNAL(readyRead()),SLOT(onProxyReadyRead()));
				connect(FRemoteSocket,SIGNAL(readyRead()),SLOT(onRemoteReadyRead()));
				emit connected(FSessionKey);
				exec();
			}
			else
			{
				setErrorCondition("remote-connect-error");
			}
		}
	}
	else
	{
		setErrorCondition("proxy-connect-error");
	}

	FThreadState = TS_DISCONNECTING;
	FProxySocket->disconnectFromHost();
	if (!FProxySocket->waitForDisconnected())
		FProxySocket->abort();

	FRemoteSocket->disconnectFromHost();
	if (!FRemoteSocket->waitForDisconnected())
		FRemoteSocket->abort();

	FThreadState = TS_DISCONNECTED;
	delete FProxySocket;
	delete FRemoteSocket;

	emit disconnected(FErrorCondition);
	deleteLater();
}

void TunnelThread::setErrorCondition(const QString &ACondition)
{
	FThreadState = TS_ERROR;
	FErrorCondition = ACondition;
}

void TunnelThread::onProxyReadyRead()
{
	FRemoteSocket->write(FProxySocket->read(FProxySocket->bytesAvailable()));
	FRemoteSocket->flush();
}

void TunnelThread::onRemoteReadyRead()
{
	FProxySocket->write(FRemoteSocket->read(FRemoteSocket->bytesAvailable()));
	FProxySocket->flush();
}
