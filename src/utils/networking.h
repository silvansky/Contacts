#ifndef NETWORKING_H
#define NETWORKING_H

#include "utilsexport.h"
#include <QUrl>
#include <QImage>

class NetworkingPrivate;

class UTILS_EXPORT Networking
{
public:
	static QImage httpGetImage(const QUrl& src);
	static void httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot);
	static bool insertPixmap(const QUrl& src, QObject* target, const QString& property = "pixmap");
	static QString httpGetString(const QUrl& src);
private:
	static NetworkingPrivate * networkingPrivate;
	static void init();
};

#endif // NETWORKING_H
